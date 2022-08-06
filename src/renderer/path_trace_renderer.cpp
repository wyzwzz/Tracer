//
// Created by wyz on 2022/5/18.
//
#include "renderer/pixel_sampler_renderer.hpp"
#include "factory/renderer.hpp"
#include "core/scene.hpp"
#include "core/material.hpp"
#include "core/sampler.hpp"
#include "core/bsdf.hpp"
#include "core/bssrdf.hpp"
#include "core/primitive.hpp"
#include "utility/logger.hpp"
#include "direct_illumination.hpp"

TRACER_BEGIN
class PathTraceRenderer:public PixelSamplerRenderer{
    int min_depth = 5;
    int max_depth = 10;
    int direct_light_sample_num = 1;
    int max_specular_depth = 20;
public:
    PathTraceRenderer(const PTRendererParams& params)
    : PixelSamplerRenderer(params.worker_count,params.task_tile_size,params.spp),
    min_depth(params.min_depth),max_depth(params.max_depth),direct_light_sample_num(params.direct_light_sample_num)
    {}

    Spectrum eval_pixel_li(const Scene& scene,const Ray& r,Sampler& sampler,MemoryArena& arena) const override{
        if(0){
            SurfaceIntersection isect;
            if (scene.intersect_p(r, &isect)) {
//                auto color = isect.material->evaluate(isect.uv);
//                return color;
                auto n = isect.shading_coord.z;
                return Spectrum(n.x + 1,n.y + 1,n.z + 1) * 0.5f;
            } else
                return Spectrum(0.0, 0.0, 0.0);
        }

        Spectrum coef(1);
        Ray ray = r;
        Spectrum L(0);
        bool specular_sample = false;

        int scattering_count = 0;

        for(int depth = 0, s_depth = 0; depth < max_depth; ++depth){
            //apply russian roulette
            Spectrum rr_coef = coef;
//            if(depth > min_depth){
//                auto q = std::max<real>(0.05,1 - rr_coef.max_component_value());
//                if(sampler.sample1().u < q) break;
//                coef /= 1 - q;
//            }
            if(depth > min_depth){
                if(sampler.sample1().u > 0.9)
                    break;
                coef /= 0.9;
            }
            if(!coef.is_valid()){
                LOG_CRITICAL("coef get invalid: {} {} {}",coef.r,coef.g,coef.b);
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
                break;
            }

            const auto medium = isect.wo_medium();
            if(scattering_count < medium->get_max_scattering_count()){
                const auto medium_sample_ret = medium->sample(r.o,isect.pos,sampler,arena,scattering_count > 0);

                coef *= medium_sample_ret.throughout;

                if(medium_sample_ret.is_scattering_happened()){
                    scattering_count++;
                    const auto& scattering_p = medium_sample_ret.scattering_point;
                    const auto phase_func = medium_sample_ret.phase_func;

                    Spectrum direct_illum;
                    for(int i = 0; i < direct_light_sample_num; ++i){
                        for(auto light:scene.lights){
                            direct_illum += coef * sample_light(scene,light,scattering_p,phase_func,sampler);
                        }
                        direct_illum += coef * sample_bsdf(scene,scattering_p,phase_func,sampler);
                    }
                    L += real(1) / direct_light_sample_num * direct_illum;

                    const auto bsdf_sample_ret = phase_func->sample(scattering_p.wo,TransportMode::Radiance,sampler.sample3());
                    if(!bsdf_sample_ret.is_valid())
                        break;

                    ray = Ray(scattering_p.pos,bsdf_sample_ret.wi);
                    coef *= bsdf_sample_ret.f / bsdf_sample_ret.pdf;
                    continue;
                }
            }
            else{
                //continent scattering too much
                //no scattering
                coef *= medium->tr(ray.o,isect.pos,sampler);
                scattering_count = 0;
            }


            auto shading_p = isect.material->shading(isect,arena);

            //sample bsdf to get new path direction
            auto bsdf_sample = shading_p.bsdf->sample(isect.wo,TransportMode::Radiance,sampler.sample3());

            if(bsdf_sample.f.is_back() || bsdf_sample.pdf < eps)
                break;

            //set specular flag for next bounce ray
            specular_sample = bsdf_sample.is_delta;//todo reduce depth due to specular sample ?
            if(specular_sample && s_depth < max_specular_depth){
                s_depth++;
                depth--;
            }

            //sample direct illumination from lights
            if(!specular_sample){
                Spectrum direct_illum;
                for(int i = 0; i < direct_light_sample_num; ++i){
                    //sample from all lights
                    for(auto light:scene.lights){
                        //todo replace with sample one light
                        direct_illum += coef * sample_light(scene,light,isect,shading_p,sampler);

                    }
                    direct_illum += coef * sample_bsdf(scene,isect,shading_p,sampler);
                }
                L += real(1) / direct_light_sample_num * direct_illum;
            }

            coef *= bsdf_sample.f * abs_cos(isect.geometry_coord.z,bsdf_sample.wi) / bsdf_sample.pdf;
            if(!coef.is_valid()){
                LOG_CRITICAL("coef get infinite: {} {} {}",coef.r,coef.g,coef.b);
                LOG_CRITICAL("bsdf sample f:{} {} {}",bsdf_sample.f.r,bsdf_sample.f.g,bsdf_sample.f.b);
                LOG_CRITICAL("bsdf sample pdf: {}",bsdf_sample.pdf);
                break;
            }
            ray = Ray(isect.eps_offset(bsdf_sample.wi),
                     normalize(bsdf_sample.wi));

            if(!shading_p.bssrdf)
                continue;


            bool pos_wi = isect.geometry_coord.in_positive_z_hemisphere(bsdf_sample.wi);
            bool pos_wo = isect.geometry_coord.in_positive_z_hemisphere(isect.wo);
            //bssrdf should only perform when ray into the inside of object
            //consider subsurface scattering only when refract happened
            if(!pos_wi && pos_wo){
                const auto bssrdf_sample_ret = shading_p.bssrdf->sample_pi(scene,sampler.sample3(),arena);
                if(bssrdf_sample_ret.coef.is_back())
                    break;

                coef *= bssrdf_sample_ret.coef / bssrdf_sample_ret.pdf;

                auto& new_isect = bssrdf_sample_ret.isect;
                auto new_shading_p = new_isect.material->shading(new_isect,arena);

                Spectrum new_direct_illum;
                for(int i = 0; i < direct_light_sample_num; i++){
                    for(auto light:scene.lights){
                        new_direct_illum += coef * sample_light(scene,light,new_isect,new_shading_p,sampler);
                    }
                    new_direct_illum += coef * sample_bsdf(scene,new_isect,new_shading_p,sampler);
                }
                L += real(1) / direct_light_sample_num * new_direct_illum;

                const auto new_bsdf_sample_ret = new_shading_p.bsdf->sample(new_isect.wo,TransportMode::Radiance,sampler.sample3());
                if(new_bsdf_sample_ret.f.is_back())
                    break;

                coef *= new_bsdf_sample_ret.f * abs_cos(new_isect.geometry_coord.z,new_bsdf_sample_ret.wi) / new_bsdf_sample_ret.pdf;

                ray = Ray(new_isect.eps_offset(new_bsdf_sample_ret.wi),new_bsdf_sample_ret.wi);

                specular_sample = new_bsdf_sample_ret.is_delta;
            }

        }
        if(!L.is_valid()){
            LOG_CRITICAL("L get infinite: {} {} {}",L.r,L.g,L.b);
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