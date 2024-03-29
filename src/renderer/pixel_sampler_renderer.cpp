//
// Created by wyz on 2022/5/18.
//

#include "pixel_sampler_renderer.hpp"
#include "utility/parallel.hpp"
#include "core/render.hpp"
#include "core/sampler.hpp"
#include "core/camera.hpp"
#include "core/scene.hpp"
#include "utility/memory.hpp"
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
        const auto scene_camera = scene.get_camera();
        const size_t total_pixels = (size_t)film_width * film_height * spp;
        std::atomic<size_t> finish_count = 0;

        auto sampler_prototype = newRC<SimpleUniformSampler>(42, false);
        PerThreadNativeSamplers perthread_sampler(
                thread_count, *sampler_prototype);
        parallel_for_2d(
                thread_count,film_width,film_height,
                tile_size,tile_size,
                [&](int thread_idx,const Bounds2i& tile_bound)
                {

                    //get sampler
                    auto sampler = perthread_sampler.get_sampler(thread_idx);


                    //get tile
                    //tile_bound is [)
                    //比如tile size是16 那么第一个区间是[0,16) 即[0,15] 第二个是[16,32) 即[16,31]
                    //但是真正的tile bounds会根据filter的radius进行校正
                    //注意传入的tile_bound是没有重叠的
                    auto film_tile = film.get_film_tile(tile_bound);

                    //create arena for each tile
                    MemoryArena arena;

                    for(Point2i pixel:tile_bound){
                        //todo re-generate sample for each spp
                        Spectrum Ls;
//                        if(pixel != Point2i(1000,1700)) continue;
                        for(int i = 0; i < spp; ++i){
                            //get camera sample to generate ray
                            const Sample2 film_sample = sampler->sample2();
                            const Sample2 lens_sample = sampler->sample2();
                            assert(film_sample.u <= 1 && film_sample.u >= 0);
                            const real pixel_x = pixel.x + film_sample.u;//uv is 0 ~ 1
                            const real pixel_y = pixel.y + film_sample.v;
                            const real film_x = pixel_x / film_width;
                            const real film_y = pixel_y / film_height;
                            CameraSample camera_sample{{film_x,film_y},{lens_sample.u,lens_sample.v}};
                            Ray ray;
                            real ray_weight = scene_camera->generate_ray(camera_sample,ray);

                            //evaluate radiance along ray
                            Spectrum L(0.0);
                            if(ray_weight > 0.0){

                                L = eval_pixel_li(scene,ray,*sampler,arena);
//                                L = {std::max(0.f,ray.d.x),std::max(0.f,ray.d.y),std::max(0.f,ray.d.z)};
                                Ls += L;
                            }
                            //add camera ray's contribution to pixel
                            //todo replace Spectrum Class
                            if(std::isfinite(L.r) && std::isfinite(L.g) && std::isfinite(L.b)){

                                film_tile->add_sample({pixel_x,pixel_y},L);

                            }
                            arena.reset();
                            finish_count++;
                            int percent = finish_count % (total_pixels / 10);
                            if( percent == 0){
                                LOG_INFO("finish {}",finish_count * 1.0 / total_pixels);
                            }
                        }

//                        if(!Ls.is_meaningful()){
//                            LOG_CRITICAL("{} {} get invalid L: {} {} {}",
//                                         pixel.x,pixel.y,Ls.r,Ls.g,Ls.b);
//                        }

                    }
                    film.merge_film_tile(film_tile);
                });

        RenderTarget render_target;
        film.write_render_target(render_target);

        return render_target;
    }



TRACER_END


