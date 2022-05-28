#include "core/renderer.hpp"
#include "core/render.hpp"
#include "core/scene.hpp"
#include "core/light.hpp"
#include "factory/filter.hpp"
#include "factory/renderer.hpp"
#include "factory/camera.hpp"
#include "factory/scene.hpp"
#include "factory/shape.hpp"
#include "factory/accelerator.hpp"
#include "factory/primivite.hpp"
#include "factory/material.hpp"
#include "factory/light.hpp"
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
    std::string ibl_file_name;

};
PTRendererParams pt_params{18,16,512,3,10,1};

void run(const RenderParams& params){
    auto filter = create_gaussin_filter(params.filter.radius,params.filter.alpha);
    auto camera = create_thin_lens_camera((real)params.render_target_width/params.render_target_height,
                                          {params.camera.pos[0],params.camera.pos[1],params.camera.pos[2]},
                                          {params.camera.target[0],params.camera.target[1],params.camera.target[2]},
                                          {params.camera.up[0],params.camera.up[1],params.camera.up[2]},
                                          params.camera.fov,
                                          params.camera.lens_radius,params.camera.fov);
    auto model = load_model_from_file(params.obj_file_name);
    std::vector<RC<Primitive>> primitives;
    LOG_INFO("load model's mesh count: {}",model.mesh.size());
    LOG_INFO("load model's material count: {}",model.material.size());
    std::vector<MaterialTexture> materials_res;
    std::vector<RC<Material>> materials;
    Span<const Light*> area_lights;
    for(auto& m:model.material){
        auto material = create_texture_from_file(m);
        materials_res.emplace_back(material);
        materials.emplace_back(create_phong_material(
                material.map_ka,
                material.map_kd,
                material.map_ks,
                material.map_ns));
    }
    LOG_INFO("load material texture count: {}",materials_res.size());
    for(const auto& mesh:model.mesh){
        assert(mesh.indices.size() % 3 == 0);
        const auto triangle_count = mesh.indices.size() / 3;
        assert(mesh.materials.size() == triangle_count);
        auto triangles = create_triangle_mesh(mesh);
        assert(triangles.size() == triangle_count);
        MediumInterface mi;
        for(size_t i = 0; i < triangle_count; ++i){
            //todo handle emission material
            assert(mesh.materials[i] < materials.size());
            const auto& material = materials[mesh.materials[i]];
            const auto& m_res = materials_res[mesh.materials[i]];
            if(materials_res[mesh.materials[i]].has_emission){
                primitives.emplace_back(create_geometric_primitive(triangles[i],material,mi,m_res.map_ke->evaluate(Point2f())));
                area_lights.emplace_back(primitives.back()->as_area_light());
            }
            else{
                primitives.emplace_back(create_geometric_primitive(triangles[i],material,mi,Spectrum()));
            }
        }
    }
    LOG_INFO("load primitives count: {}",primitives.size());
    auto bvh = create_bvh_accel(3);
    {
        AutoTimer timer("bvh build");
        bvh->build(std::move(primitives));
    }
    auto scene = create_general_scene(bvh);
    scene->lights = area_lights;
    scene->set_camera(camera);
    if(!params.ibl_file_name.empty()){
        auto env_map = create_texture2d_from_file("C:/Users/wyz/projects/RayTracer/data/CG2020-master/car/environment_day.hdr");
        auto ibl = create_ibl_light(env_map, rotate_x(PIOver2_r));
        scene->environment_light = ibl;
        scene->lights.push_back(ibl.get());
    }
    scene->prepare_to_render();
    auto renderer = create_pt_renderer(pt_params);
    AutoTimer timer("render","s");
    auto render_target = renderer->render(*scene.get(), Film({params.render_target_width, params.render_target_height}, filter));
    write_image_to_hdr(render_target.color, params.render_result_name+"hdr");
    LOG_INFO("write hdr...");
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
int main(int argc,char** argv){
    RenderParams bedroom = {
        .render_result_name = "tracer_bedroom",
        .filter = {.radius = 0.5,.alpha = 0.6},
        .render_target_width = 1280,
        .render_target_height = 720,
        .camera = {
                .pos = {3.456,1.212,3.299},
                .target = {2.699,1.195,2.645},
                .up = {-0.013,1.000,-0.011},
                .fov = PI_r * 39.4305 / 180.0,
                .lens_radius = 0,
                .focal_dist = 10.0
        },
        .obj_file_name = "bedroom.obj",
    };

    RenderParams cornell_box = {
        .render_result_name = "tracer_cornel-box",
        .filter = {.radius = 0.5,.alpha = 0.6},
        .render_target_width = 1024,
        .render_target_height = 1024,
        .camera = {
                .pos = {0,1,6.8},
                .target = {0,1,5.8},
                .up = {0,1,0},
                .fov = PI_r * 19.5 / 180.0,
                .lens_radius = 0,
                .focal_dist = 10.0
        },
        .obj_file_name = "cornell-box.obj"
    };

    RenderParams veach_mis = {
        .render_result_name = "tracer_veach-mis",
        .filter = {.radius = 0.5,.alpha = 0.6},
        .render_target_width = 1200,
        .render_target_height = 900,
        .camera = {
                .pos = {0.0,2.0,15.0},
                .target = {0.0,1.69521,14.0476},
                .up = {0.0,0.952421,-0.304787},
                .fov = PI_r * 27.3909 / 180.0,
                .lens_radius = 0,
                .focal_dist = 10.0
        },
        .obj_file_name = "veach-mis.obj"
    };
    try{
        run(cornell_box);
    }
    catch(const std::exception& e){
        LOG_CRITICAL("exception: {}",e.what());
    }
    return 0;
}