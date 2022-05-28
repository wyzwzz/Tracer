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
#include "utility/logger.hpp"
#include "direct_illumination.hpp"
TRACER_BEGIN
class PathTraceRenderer:public PixelSamplerRenderer{
    int min_depth = 5;
    int max_depth = 10;
    int direct_light_sample_num = 1;
public:
    PathTraceRenderer(const PTRendererParams& params)
    : PixelSamplerRenderer(params.worker_count,params.task_tile_size,params.spp),
    min_depth(params.min_depth),max_depth(params.max_depth),direct_light_sample_num(params.direct_light_sample_num)
    {}

    Spectrum eval_pixel_li(const Scene& scene,const Ray& r,Sampler& sampler,MemoryArena& arena) const override{
        if(0){
            SurfaceIntersection isect;
            if (scene.intersect_p(r, &isect)) {
                auto color = isect.material->evaluate(isect.uv);
                return color;
                auto n = isect.shading.n;
                return Spectrum(n.x + 1,n.y + 1,n.z + 1) * 0.5f;
            } else
                return Spectrum(0.0, 0.0, 0.0);
        }

        Spectrum coef(1);
        Ray ray = r;
        Spectrum L(0);
        bool specular_sample = false;

        for(int depth = 0; depth < max_depth; ++depth){
            if(!coef.is_finite()){
                LOG_CRITICAL("coef get infinite");
                break;
            }
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

                        L += coef * light->light_emit(ray.o,ray.d);
                    }
                }
            }

            if(!found_intersection){
//                return Spectrum(1,0,0);
                break;
            }

            auto shading_p = isect.material->shading(isect,arena);

            //sample direct illumination from lights
            if(!specular_sample){
                Spectrum direct_illum;
                for(int i = 0; i < direct_light_sample_num; ++i){
                    //sample from all lights
                    for(auto light:scene.lights){
                        direct_illum += coef * sample_light(scene,light,isect,shading_p,sampler);

                    }
                    direct_illum += coef * sample_bsdf(scene,isect,shading_p,sampler);
                }
                L += real(1) / direct_light_sample_num * direct_illum;
            }


            //sample bsdf to get new path direction
            auto bsdf_sample = shading_p.bsdf->sample(isect.wo,sampler.sample3());



            if(bsdf_sample.f.is_back() || bsdf_sample.pdf < eps){

                break;
            }


            //set specular flag for next bounce ray
            specular_sample = bsdf_sample.is_delta;//todo reduce depth due to specular sample ?
            if(specular_sample){
                depth--;
            }
            //todo handle refract?

            const real abscos = abs_dot(isect.shading.n,bsdf_sample.wi);
            coef *= bsdf_sample.f * abscos / bsdf_sample.pdf;

             ray = Ray(isect.eps_offset(bsdf_sample.wi),
                     normalize(bsdf_sample.wi));

            //todo handle medium or bssdf ?

            //apply russian roulette
            Spectrum rr_coef = coef;
            if(depth > min_depth){
                auto q = std::max<real>(0.05,1 - rr_coef.max_component_value());
                if(sampler.sample1().u < q) break;
                coef /= 1 - q;
            }
//            if(depth > min_depth){
//                if(sampler.sample1().u > 0.9)
//                    break;
//                coef /= 0.9;
//            }
        }
        if(!L.is_finite()){
            LOG_CRITICAL("L get infinite");
            return {};
        }
        else
            return L;
    }

};





    RC<Renderer> create_pt_renderer(const PTRendererParams& params){
        return newBox<PathTraceRenderer>(params);
    }

TRACER_END