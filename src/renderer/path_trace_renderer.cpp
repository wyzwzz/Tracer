//
// Created by wyz on 2022/5/18.
//
#include "renderer/pixel_sampler_renderer.hpp"
#include "factory/renderer.hpp"
#include "core/scene.hpp"
#include "core/material.hpp"
#include "core/sampler.hpp"
#include "core/bsdf.hpp"
#include "core/primitive.hpp"
#include "direct_illumination.hpp"
TRACER_BEGIN
class PathTraceRenderer:public PixelSamplerRenderer{
    int max_depth = 10;
    int min_depth = 3;
    int direct_light_sample_num = 1;
public:
    PathTraceRenderer(const PTRendererParams& params)
    : PixelSamplerRenderer(params.worker_count,params.task_tile_size,params.spp)
    {}

    Spectrum eval_pixel_li(const Scene& scene,const Ray& ray,Sampler& sampler,MemoryArena& arena) const override{
        if(false){
            SurfaceIntersection isect;
            if (scene.intersect_p(ray, &isect)) {
                auto color = isect.material->evaluate(isect.uv);
                return color;
            } else
                return Spectrum(0.0, 0.0, 0.0);
        }

        Spectrum coef(1);
        Ray r = ray;
        Spectrum L(0);
        bool specular_sample = false;

        for(int depth = 0; depth < max_depth; ++depth){

            SurfaceIntersection isect;
            bool found_intersection = scene.intersect_p(ray,&isect);

            if(depth == 0  || specular_sample)//todo specular ?
            {
                if(found_intersection){
                    //if intersect with emit object
                    if(auto light = isect.primitive->as_area_light()){
                        //add emission radiance from hit area light
                        L += coef * light->light_emit(isect,-ray.d);
                    }
                }
                else{
                    //sample from environment light
                    if(auto light = scene.environment_light.get()){
                        //todo
                        L += coef * light->light_emit();
                    }
                }
            }

            if(!found_intersection) break;

            auto shading_p = isect.material->shading(isect,arena);

            //sample direct illumination from lights
            if(!specular_sample){
                Spectrum direct_illum;
                for(int i = 0; i < direct_light_sample_num; ++i){
                    //sample from all lights
                    for(auto light:scene.lights){
                        direct_illum += coef * uniform_sample_light(scene,light.get(),isect,shading_p,sampler);
                    }
                    //uniform_sample_light完成对光源和bsdf的多重重要性采样
                }
                L += real(1) / direct_light_sample_num * direct_illum;
            }


            //sample bsdf to get new path direction
            auto bsdf_sample = shading_p.bsdf->sample(isect.wo,sampler.sample3());

            if(bsdf_sample.f.is_back() || bsdf_sample.pdf < eps) break;

            //set specular flag for next bounce ray
            specular_sample = bsdf_sample.is_delta;//todo reduce depth due to specular sample ?
            //todo handle refract?

            const real abscos = abs_dot(isect.map_n,bsdf_sample.wi);
            coef *= bsdf_sample.f * abscos / bsdf_sample.pdf;

             r = Ray(isect.eps_offset(bsdf_sample.wi),
                     normalize(bsdf_sample.wi));

            //todo handle medium or bssdf ?

            //apply russian roulette
            Spectrum rr_coef = coef;
            if(depth > 3){
                auto q = std::max<real>(0.05,1 - rr_coef.max_component_value());
                if(sampler.sample1().u < q) break;
                coef /= q;
            }
        }
        return L;
    }

};





    RC<Renderer> create_pt_renderer(const PTRendererParams& params){
        return newBox<PathTraceRenderer>(params);
    }

TRACER_END