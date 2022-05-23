//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_PIXEL_SAMPLER_RENDERER_HPP
#define TRACER_PIXEL_SAMPLER_RENDERER_HPP

#include "core/renderer.hpp"
#include "utility/memory.hpp"
TRACER_BEGIN


class PixelSamplerRenderer: public Renderer{
public:
    PixelSamplerRenderer(int worker_count,int tile_size = 16,int spp = 1);

    ~PixelSamplerRenderer() override;

    RenderTarget render(const Scene& scene,Film film) override;

protected:
    virtual Spectrum eval_pixel_li(const Scene& scene,const Ray& ray,Sampler& sampler,MemoryArena& arena) const = 0;
private:
    int worker_count;
    int tile_size;
    int spp;
};

TRACER_END

#endif //TRACER_PIXEL_SAMPLER_RENDERER_HPP
