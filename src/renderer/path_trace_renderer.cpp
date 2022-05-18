//
// Created by wyz on 2022/5/18.
//
#include "renderer/pixel_sampler_renderer.hpp"
#include "factory/renderer.hpp"
TRACER_BEGIN
class PathTraceRenderer:public PixelSamplerRenderer{
public:
    PathTraceRenderer(const PTRendererParams& params)
    : PixelSamplerRenderer(params.worker_count,params.task_tile_size,params.spp)
    {}

    Spectrum eval_pixel_li(const Scene& scene,const Ray& ray) const override{
        return Spectrum(1.f);
    }

};
    RC<Renderer> create_pt_renderer(const PTRendererParams& params){
        return newBox<PathTraceRenderer>(params);
    }

TRACER_END