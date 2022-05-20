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
#include "utility/image_file.hpp"
#include "utility/logger.hpp"
#include "utility/timer.hpp"
using namespace tracer;
int main(int argc,char** argv){
    auto filter = create_gaussin_filter(0.5,0.6);
    int image_w = 1280;
    int image_h = 720;

    PTRendererParams params{10,16,2};
    auto renderer = create_pt_renderer(params);
    auto camera = create_thin_lens_camera((real)1280/720,
                                          {0.0,0.0,-1.0},
                                          {0.0,0.0,0.0},
                                          {0.0,1.0,0.0},
                                          PI_r*0.5,
                                          0.025,1);

    auto model = load_model_from_file("C:/Users/wyz/projects/RayTracer/data/CG2020-master/car/car.obj");
    std::vector<RC<Primitive>> primitives;
    LOG_INFO("load model's mesh count: {}",model.mesh.size());
    for(const auto& mesh:model.mesh){
        assert(mesh.indices.size() % 3 == 0);
        const auto triangle_count = mesh.indices.size() / 3;
//        assert(mesh.materials.size() == triangle_count);
        auto triangles = create_triangle_mesh(mesh);
        assert(triangles.size() == triangle_count);
        MediumInterface mi;
        for(size_t i = 0; i < triangle_count; ++i){
            //todo handle emission material
            primitives.emplace_back(create_geometric_primitive(triangles[i],nullptr,mi));
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
    auto render_target = renderer->render(*scene.get(),Film({image_w,image_h},filter));
    write_image_to_hdr(render_target.color,"tracer_test.hdr");

    return 0;
}