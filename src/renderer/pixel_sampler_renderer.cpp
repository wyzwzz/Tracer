//
// Created by wyz on 2022/5/18.
//

#include "pixel_sampler_renderer.hpp"
#include "utility/parallel.hpp"
#include "core/render.hpp"
#include "core/sampler.hpp"
TRACER_BEGIN

    PixelSamplerRenderer::PixelSamplerRenderer(int worker_count, int tile_size, int spp)
    :worker_count(worker_count),tile_size(tile_size),spp(spp)
    {
        assert(worker_count >= 0 && tile_size > 0 && spp > 0);
    }

    PixelSamplerRenderer::~PixelSamplerRenderer() {

    }

    RenderTarget PixelSamplerRenderer::render(
            const Scene &scene,Film film) {
        const int thread_count = actual_worker_count(worker_count);
        const int film_width = film.width();
        const int film_height = film.height();
        parallel_for_2d(
                thread_count,film_width,film_height,
                tile_size,tile_size,
                [&](int thread_idx,const Bounds2i& tile_bound)
                {

                    //get sampler
                    auto sampler = newBox<SimpleUniformSampler>(thread_idx);

                    //get tile
                    //tile_bound is [)
                    //比如tile size是16 那么第一个区间是[0,16) 即[0,15] 第二个是[16,32) 即[16,31]
                    //但是真正的tile bounds会根据filter的radius进行校正
                    //注意传入的tile_bound是没有重叠的
                    auto film_tile = film.get_film_tile(tile_bound);

                    //todo use arena

                    for(Point2i pixel:tile_bound){
                        for(int i = 0; i < spp; ++i){
                            //get camera sample to generate ray
                            const Sample2 film_sample = sampler->sample2();


                            //evaluate radiance along ray

                            //add camera ray's contribution to pixel

                            film_tile->add_sample((Point2f)pixel,{1.0,1.0,0.0});

                        }
                    }
                    film.merge_film_tile(film_tile);
                });

        RenderTarget render_target;
        film.write_render_target(render_target);

        return render_target;
    }



TRACER_END


