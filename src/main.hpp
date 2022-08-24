#include "core/renderer.hpp"
#include "core/render.hpp"
#include "core/scene.hpp"
#include "core/light.hpp"
#include "core/bssrdf.hpp"
#include "factory/filter.hpp"
#include "factory/renderer.hpp"
#include "factory/camera.hpp"
#include "factory/scene.hpp"
#include "factory/shape.hpp"
#include "factory/accelerator.hpp"
#include "factory/primivite.hpp"
#include "factory/material.hpp"
#include "factory/light.hpp"
#include "factory/post_processor.hpp"
#include "factory/medium.hpp"
#include "factory/texture.hpp"
#include "utility/image_file.hpp"
#include "utility/logger.hpp"
#include "utility/timer.hpp"
#include <stdexcept>
#include <array>
using namespace tracer;

struct RenderParams{
    std::string render_result_name;
    struct{
        real radius;
        real alpha;
    }filter;
    int render_target_width;
    int render_target_height;
    struct{
        std::array<real,3> pos;
        std::array<real,3> target;
        std::array<real,3> up;
        real fov;
        real lens_radius;
        real focal_dist;
    }camera;
    std::string obj_file_name;
    mutable std::string ibl_file_name;

};

struct DisneyBRDFParams{
    Spectrum base_color;
    real subsurface;
    real metallic;
    real specular;
    real specular_tint;
    real roughness;
    real anisotropic;
    real sheen;
    real sheen_tint;
    real clear_coat;
    real clearcoat_gloss;
};

PTRendererParams pt_params{
        .worker_count = 18,
        .task_tile_size = 16,
        .spp = 1024,
        .min_depth = 5,
        .max_depth = 10,
        .direct_light_sample_num = 1};

SPPMRendererParams sppm_params{.init_search_radius = 0.25,
        .worker_count = 18,
        .iteration_count = 4096,
        .photons_per_iteration = 120000,
        .task_tile_size = 16,
        .ray_trace_max_depth = 8,
        .photon_min_depth = 5,
        .photon_max_depth = 10,
        .update_alpha = real(2)/3
};

BDPTRendererParams bdpt_params{
        .worker_count = 18,
        .task_tile_size = 16,
        .max_camera_vertex_count = 10,
        .max_light_vertex_count = 10,
        .spp =  1024
};

DisneyBRDFParams disney_brdf_params = {
        .base_color = {0.82,0.82,0.82},
        .subsurface = 1.0,
        .metallic = 0.0,
        .specular = 1.0,
        .specular_tint = 1,
        .roughness = 0.0,
        .anisotropic = 0,
        .sheen = 0.0,
        .sheen_tint = 0.5,
        .clear_coat = 0.0,
        .clearcoat_gloss = 1
};

void run_disney_brdf(const RenderParams& params,const DisneyBRDFParams& disney){
    auto filter = create_gaussin_filter(params.filter.radius,params.filter.alpha);
    auto camera = create_thin_lens_camera((real)params.render_target_width/params.render_target_height,
                                          {params.camera.pos[0],params.camera.pos[1],params.camera.pos[2]},
                                          {params.camera.target[0],params.camera.target[1],params.camera.target[2]},
                                          {params.camera.up[0],params.camera.up[1],params.camera.up[2]},
                                          params.camera.fov,
                                          params.camera.lens_radius,params.camera.fov);
    auto model = load_model_from_file(params.obj_file_name);
    auto vacuum = create_vacuum();

    MediumInterface mi;
    mi.outside = vacuum;
    mi.inside = vacuum;

//    assert(model.material.size() == 1);
//    auto albedo = create_texture_from_file(model.material.front()).map_kd;
    auto disney_material = create_disney_brdf(
            create_constant_texture2d(disney.base_color),
//            albedo,
            create_constant_texture2d(Spectrum(disney.subsurface)),
            create_constant_texture2d(Spectrum(disney.metallic)),
            create_constant_texture2d(Spectrum(disney.specular)),
            create_constant_texture2d(Spectrum(disney.specular_tint)),
            create_constant_texture2d(Spectrum(disney.roughness)),
            create_constant_texture2d(Spectrum(disney.anisotropic)),
            create_constant_texture2d(Spectrum(disney.sheen)),
            create_constant_texture2d(Spectrum(disney.sheen_tint)),
            create_constant_texture2d(Spectrum(disney.clear_coat)),
            create_constant_texture2d(Spectrum(disney.clearcoat_gloss))
            );

    auto ball_primitive = create_geometric_primitive(create_sphere(1.6,Transform()),disney_material,mi,Spectrum(0));
    std::vector<RC<Primitive>> primitives;
//    primitives.emplace_back(ball_primitive);
    Transform model_transform  = translate({0,0,-2}) * rotate_x(PI_r / 2);//= translate({0,0,-2}) * scale(2,2,2) * rotate_x(PI_r);//
    for(const auto& mesh:model.mesh){
        const auto triangle_count = mesh.indices.size() / 3;
        auto triangles = create_triangle_mesh(mesh,model_transform);

        for(size_t i = 0; i < triangle_count; ++i){
            primitives.emplace_back(create_geometric_primitive(
                    triangles[i],disney_material,mi,Spectrum()
                    ));
        }
    }
    auto bvh = create_bvh_accel(3);
    {
        AutoTimer timer("bvh build");
        bvh->build(std::move(primitives));
    }
    auto scene = create_general_scene(bvh);
    scene->set_camera(camera);
    if(!params.ibl_file_name.empty()){
        auto env_map = create_texture2d_from_file(params.ibl_file_name);
        auto ibl = create_ibl_light(env_map, rotate_x(0));
        scene->environment_light = ibl;
        scene->lights.push_back(ibl.get());
    }
    scene->prepare_to_render();
    auto renderer = create_pt_renderer(pt_params);

    AutoTimer timer("render","s");
    auto render_target = renderer->render(*scene.get(), Film({params.render_target_width, params.render_target_height}, filter));
    write_image_to_hdr(render_target.color, params.render_result_name+".hdr");
    LOG_INFO("write hdr...");
    auto gamma_corrector = create_gamma_corrector(1.0/2.2);
    auto aces_tone_mapper = create_aces_tone_mapper(1);
//    aces_tone_mapper->process(render_target);
    auto& imgf = render_target.color;
    Image2D<Color3b> imgu8(imgf.width(),imgf.height());
    real inv_gamma = 1.0 / 2.2;
    for(int i = 0; i < imgf.width(); i++){
        for(int j = 0; j < imgf.height(); j++){
            imgu8.at(i,j).x = std::clamp<int>(std::pow(imgf.at(i,j).r,inv_gamma) * 255,0,255);
            imgu8.at(i,j).y = std::clamp<int>(std::pow(imgf.at(i,j).g,inv_gamma) * 255,0,255);
            imgu8.at(i,j).z = std::clamp<int>(std::pow(imgf.at(i,j).b,inv_gamma) * 255,0,255);
        }
    }
    write_image_to_png(imgu8,params.render_result_name+".png");
    LOG_INFO("write png...");
    LOG_INFO("finish task");
}