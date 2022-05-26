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
#include "utility/image_file.hpp"
#include "utility/logger.hpp"
#include "utility/timer.hpp"
#include <stdexcept>
using namespace tracer;
int main(int argc,char** argv){
    auto filter = create_gaussin_filter(0.5,0.6);
    int image_w = 480;
    int image_h = 270;

    PTRendererParams params{18,32,512,8,16,4};
    auto renderer = create_pt_renderer(params);
    auto camera = create_thin_lens_camera((real)image_w/image_h,
                                          {0,12.72,31.85},
                                          {0,12.546,30.865},
                                          {0,0.985,-0.174},
                                          PI_r*45.0/180.0,
                                          0.0,10);

//    auto camera = create_thin_lens_camera((real)image_w/image_h,
//                                          {0,0,2.5},
//                                          {0,0,0},
//                                          {0,1,0},
//                                          PI_r*60.0/180.0,
//                                          0.0,10);

    auto model = load_model_from_file("C:/Users/wyz/projects/RayTracer/data/CG2020-master/diningroom/diningroom.obj");
//    auto model = load_model_from_file("C:/Users/wyz/projects/RayTracer/data/CG2020-master/cornellbox/cornellbox.obj");
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
//            primitives.emplace_back(create_geometric_primitive(triangles[i],material,mi,
//                                                               m_res.has_emission ?
//                                                               m_res.map_ke->evaluate(Point2f()) :
//                                                               Spectrum()));
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
    try{
        AutoTimer timer("render","s");
        auto render_target = renderer->render(*scene.get(), Film({image_w, image_h}, filter));
        write_image_to_hdr(render_target.color, "tracer_test_diningroom.hdr");
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
        write_image_to_png(imgu8,"tracer_test_diningroom.png");
        LOG_INFO("write png...");
    }
    catch(const std::exception& e){
        LOG_CRITICAL("exception: {}",e.what());
    }
    return 0;
}