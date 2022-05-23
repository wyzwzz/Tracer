#include "core/renderer.hpp"
#include "core/render.hpp"
#include "core/scene.hpp"
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
using namespace tracer;
int main(int argc,char** argv){
    auto filter = create_gaussin_filter(0.5,0.6);
    int image_w = 1920;
    int image_h = 1080;

    PTRendererParams params{10,16,16};
    auto renderer = create_pt_renderer(params);
    auto camera = create_thin_lens_camera((real)image_w/image_h,
                                          {0,12.72,31.85},
                                          {0,12.546,30.865},
                                          {0,0.985,-0.174},
                                          PI_r*45.0/180.0,
                                          0.0,10);

    auto model = load_model_from_file("C:/Users/wyz/projects/RayTracer/data/CG2020-master/diningroom/diningroom.obj");
    std::vector<RC<Primitive>> primitives;
    LOG_INFO("load model's mesh count: {}",model.mesh.size());
    LOG_INFO("load model's material count: {}",model.material.size());
    std::vector<MaterialTexture> materials_res;
    std::vector<RC<Material>> materials;
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
            primitives.emplace_back(create_geometric_primitive(triangles[i],material,mi,
                                                               materials_res[mesh.materials[i]].has_emission ?
                                                               materials_res[mesh.materials[i]].map_ke->evaluate(Point2f()) :
                                                               Spectrum()));
        }
    }
    LOG_INFO("load primitives count: {}",primitives.size());
    auto bvh = create_bvh_accel(3);
    {
        AutoTimer timer("bvh build");
        bvh->build(std::move(primitives));
    }
    auto scene = create_general_scene(bvh);
    scene->set_camera(camera);
    {
        AutoTimer timer("render");
        auto render_target = renderer->render(*scene.get(), Film({image_w, image_h}, filter));
        write_image_to_hdr(render_target.color, "tracer_test.hdr");
    }

    return 0;
}