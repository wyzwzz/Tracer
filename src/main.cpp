#include "core/renderer.hpp"
#include "core/render.hpp"
#include "core/scene.hpp"
#include "factory/filter.hpp"
#include "factory/renderer.hpp"
#include "factory/camera.hpp"
#include "factory/scene.hpp"
#include "utility/image_file.hpp"
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
    auto scene = create_general_scene();
    scene->set_camera(camera);
    auto render_target = renderer->render(*scene.get(),Film({image_w,image_h},filter));
    write_image_to_hdr(render_target.color,"tracer_test.hdr");

    return 0;
}