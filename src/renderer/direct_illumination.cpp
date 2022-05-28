//
// Created by wyz on 2022/5/23.
//
#include "direct_illumination.hpp"
#include "core/scene.hpp"
#include "core/sampling.hpp"
#include "core/primitive.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN

    Spectrum sample_light(const Scene& scene,const Light* light,
                          const SurfaceIntersection& isect,
                          const SurfaceShadingPoint& shd_p,
                          Sampler& sampler){
        if(light->as_area_light())
            return sample_area_light(scene,light->as_area_light(),isect,shd_p,sampler);
        else if(light->as_environment_light())
            return sample_environment_light(scene,light->as_environment_light(),isect,shd_p,sampler);
        else
            return {};
    }

    Spectrum sample_area_light(const Scene& scene,const AreaLight* light,
                               const SurfaceIntersection& isect,
                               const SurfaceShadingPoint& shd_p,
                               Sampler& sampler){
        const Sample5 sample = sampler.sample5();

        auto light_sample = light->sample_li(isect,sample);
        if(!light_sample.radiance || !light_sample.pdf)
            return {};


        auto isect_to_light = light_sample.pos - isect.pos;
        auto dist = isect_to_light.length() - eps;
        if(dist <= eps)
            return {};
        isect_to_light = normalize(isect_to_light);
        const Ray shadow_ray(isect.pos,isect_to_light,eps,dist);
        if(scene.intersect(shadow_ray))
            return {};
        //todo handle medium ?
        const auto bsdf_f = shd_p.bsdf->eval(isect_to_light,isect.wo);
        //
        if(!bsdf_f)
            return {};
        //todo trace ray in medium

        const real bsdf_pdf = shd_p.bsdf->pdf(isect_to_light,isect.wo);

        const Spectrum f = light_sample.radiance * bsdf_f *
                abs_dot(isect_to_light,isect.shading.n);//todo ???

        float weight = PowerHeuristic(1,light_sample.pdf,1,bsdf_pdf);

//        return f / (light_sample.pdf + bsdf_pdf);
        return  f * weight / light_sample.pdf;//pbrt
    }

    Spectrum sample_environment_light(const Scene& scene,const EnvironmentLight* light,
                                      const SurfaceIntersection& isect,
                                      const SurfaceShadingPoint& shd_p,
                                      Sampler& sampler){
        Sample5 sample = sampler.sample5();

        auto light_sample_ret = light->sample_li(isect,sample);
        if(!light_sample_ret.radiance)
            return {};

        auto isect_to_light = light_sample_ret.pos - isect.pos;
        auto dist = isect_to_light.length() - eps;
        if(dist <= eps)
            return {};
        isect_to_light = normalize(isect_to_light);
        const Ray shadow_ray(isect.pos,isect_to_light,eps,dist);
        if(scene.intersect(shadow_ray))
            return {};

        const auto bsdf_f = shd_p.bsdf->eval(isect_to_light,isect.wo);

        if(!bsdf_f)
            return {};

        const real bsdf_pdf = shd_p.bsdf->pdf(isect_to_light,isect.wo);

        //no need to handle medium

        const Spectrum f = light_sample_ret.radiance * bsdf_f *
                           abs_dot(isect_to_light,isect.shading.n);//todo ???

        float weight = PowerHeuristic(1,light_sample_ret.pdf,1,bsdf_pdf);

//        return f / (light_sample.pdf + bsdf_pdf);
        return  f * weight / light_sample_ret.pdf;//pbrt

    }


    Spectrum sample_bsdf(const Scene& scene,const SurfaceIntersection& isect,
                         const SurfaceShadingPoint& shd_p,Sampler& sampler){
        const Sample3 sample = sampler.sample3();

        auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,sample);
        if(!bsdf_sample_ret.is_valid())
            return {};
        bsdf_sample_ret.wi = normalize(bsdf_sample_ret.wi);

        Ray ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);
        SurfaceIntersection t_isect;
        bool has_intersect = scene.intersect_p(ray,&t_isect);
        if(!has_intersect){
            //sample from environment light
            Spectrum envir_illum;
            if(auto light = scene.environment_light){
                Spectrum envir_radiance = light->light_emit(ray.o,ray.d);
                if(!envir_radiance)
                    return {};

                Spectrum f = envir_radiance * bsdf_sample_ret.f * abs_dot(isect.n,ray.d);
                if(bsdf_sample_ret.is_delta)
                    envir_illum = f / bsdf_sample_ret.pdf;
                else{
                    auto light_pdf = light->pdf(ray.o,ray.d);
                    auto weight = PowerHeuristic(1,bsdf_sample_ret.pdf,1,light_pdf);
                    envir_illum = f * weight / bsdf_sample_ret.pdf;
                }
            }
//            LOG_INFO("envir_illum : {} {} {}",envir_illum.r,envir_illum.g,envir_illum.b);
//            LOG_INFO("envir illum bsdf pdf: {}",bsdf_sample_ret.pdf);
            return envir_illum;
        }

        auto light = t_isect.primitive->as_area_light();
        if(!light)
            return {};

        Spectrum light_radiance = light->light_emit(t_isect,t_isect.wo);
        if(!light_radiance)
            return {};

        //todo handle medium ?

        Spectrum f = light_radiance * bsdf_sample_ret.f * abs_dot(isect.shading.n,ray.d);

        if(bsdf_sample_ret.is_delta)
            return f / bsdf_sample_ret.pdf;

        real light_pdf = light->pdf(ray.o,t_isect.pos,t_isect.n);

        real weight = PowerHeuristic(1,bsdf_sample_ret.pdf,1,light_pdf);

//        return f / (bsdf_sample_ret.pdf + light_pdf);
        return f * weight / bsdf_sample_ret.pdf;//pbrt
    }
TRACER_END