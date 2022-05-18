#include "core/renderer.hpp"
#include "core/render.hpp"
#include "core/scene.hpp"
#include "factory/filter.hpp"
#include "factory/renderer.hpp"
#include "utility/image_file.hpp"
using namespace tracer;
int main(int argc,char** argv){
    auto filter = create_gaussin_filter(0.5,0.6);
    int image_w = 1280;
    int image_h = 720;

    PTRendererParams params;
    auto renderer = create_pt_renderer(params);
    Scene scene;
    auto render_target = renderer->render(scene,Film({image_w,image_h},filter));
    write_image_to_hdr(render_target.color,"tracer_test.hdr");

    return 0;
}