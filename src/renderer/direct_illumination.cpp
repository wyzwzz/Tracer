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

    Spectrum sample_light(const Scene& scene,const Light* light,
                          const MediumScatteringP& scattering_p,
                          const BSDF* phase_func,
                          Sampler& sampler){
        if(light->as_area_light())
            return sample_area_light(scene,light->as_area_light(),scattering_p,phase_func,sampler);
        else if(light->as_environment_light())
            return sample_environment_light(scene,light->as_environment_light(),scattering_p,phase_func,sampler);
        else
            return {};
    }

    Spectrum sample_area_light(const Scene& scene,const AreaLight* light,
                               const SurfaceIntersection& isect,
                               const SurfaceShadingPoint& shd_p,
                               Sampler& sampler){
        const Sample5 sample = sampler.sample5();

        auto light_sample = light->sample_li(isect.pos,sample);
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

        const auto medium = isect.medium(isect_to_light);

        const auto bsdf_f = shd_p.bsdf->eval(isect_to_light,isect.wo,TransportMode::Radiance);

        if(!bsdf_f)
            return {};

        const real bsdf_pdf = shd_p.bsdf->pdf(isect_to_light,isect.wo);

        const Spectrum f = medium->tr(light_sample.pos,isect.pos,sampler) * light_sample.radiance * bsdf_f *
                abs_cos(isect_to_light,isect.geometry_coord.z);

        float weight = PowerHeuristic(1,light_sample.pdf,1,bsdf_pdf);

//        return f / (light_sample.pdf + bsdf_pdf);
        return  f * weight / light_sample.pdf;//pbrt
    }

    Spectrum sample_environment_light(const Scene& scene,const EnvironmentLight* light,
                                      const SurfaceIntersection& isect,
                                      const SurfaceShadingPoint& shd_p,
                                      Sampler& sampler){
        Sample5 sample = sampler.sample5();

        auto light_sample_ret = light->sample_li(isect.pos,sample);
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

        const auto bsdf_f = shd_p.bsdf->eval(isect_to_light,isect.wo,TransportMode::Radiance);

        if(!bsdf_f)
            return {};

        const real bsdf_pdf = shd_p.bsdf->pdf(isect_to_light,isect.wo);

        //no need to handle medium

        const Spectrum f = light_sample_ret.radiance * bsdf_f *
                           abs_cos(isect_to_light,isect.geometry_coord.z);

        float weight = PowerHeuristic(1,light_sample_ret.pdf,1,bsdf_pdf);

//        return f / (light_sample.pdf + bsdf_pdf);
        return  f * weight / light_sample_ret.pdf;//pbrt

    }

    Spectrum sample_area_light(const Scene& scene,const AreaLight* light,
                               const MediumScatteringP& scattering_p,
                               const BSDF* phase_func,
                               Sampler& sampler){
        const Sample5 sample = sampler.sample5();

        auto light_sample = light->sample_li(scattering_p.pos,sample);
        if(!light_sample.radiance || !light_sample.pdf)
            return {};

        if(!scene.visible(light_sample.pos,scattering_p.pos))
            return {};

        const auto isect_to_light = (light_sample.pos - scattering_p.pos).normalize();
        auto bsdf_f = phase_func->eval(isect_to_light,scattering_p.wo,TransportMode::Radiance);
        if(!bsdf_f)
            return {};

        const auto medium = scattering_p.medium;
        Spectrum f = medium->tr(scattering_p.pos,light_sample.pos,sampler) * bsdf_f * light_sample.radiance;//no cos theta
        real bsdf_pdf = phase_func->pdf(isect_to_light,scattering_p.wo);
        float weight = PowerHeuristic(1,light_sample.pdf,1,bsdf_pdf);
        return f * weight / light_sample.pdf;
    }

    Spectrum sample_environment_light(const Scene& scene,const EnvironmentLight* light,
                                      const MediumScatteringP& scattering_p,
                                      const BSDF* phase_func,
                                      Sampler& sampler){
        return {};
    }


    Spectrum sample_bsdf(const Scene& scene,const SurfaceIntersection& isect,
                         const SurfaceShadingPoint& shd_p,Sampler& sampler){
        const Sample3 sample = sampler.sample3();

        auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,TransportMode::Radiance,sample);
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

                Spectrum f = envir_radiance * bsdf_sample_ret.f * abs_cos(isect.geometry_coord.z,ray.d);
                if(bsdf_sample_ret.is_delta)
                    envir_illum = f / bsdf_sample_ret.pdf;
                else{
                    auto light_pdf = light->pdf(ray.o,ray.d);
                    auto weight = PowerHeuristic(1,bsdf_sample_ret.pdf,1,light_pdf);
                    envir_illum = f * weight / bsdf_sample_ret.pdf;
                }
            }
            return envir_illum;
        }

        auto light = t_isect.primitive->as_area_light();
        if(!light)
            return {};

        Spectrum light_radiance = light->light_emit(t_isect,t_isect.wo);
        if(!light_radiance)
            return {};

        const auto medium = isect.medium(bsdf_sample_ret.wi);

        Spectrum f = medium->tr(ray.o,isect.pos,sampler) *
                light_radiance * bsdf_sample_ret.f * abs_cos(isect.geometry_coord.z,ray.d);

        if(bsdf_sample_ret.is_delta)
            return f / bsdf_sample_ret.pdf;

        real light_pdf = light->pdf(ray.o,t_isect.pos,(Normal3f)t_isect.geometry_coord.z);

        real weight = PowerHeuristic(1,bsdf_sample_ret.pdf,1,light_pdf);

//        return f / (bsdf_sample_ret.pdf + light_pdf);
        return f * weight / bsdf_sample_ret.pdf;//pbrt
    }

    Spectrum sample_bsdf(const Scene& scene,const MediumScatteringP& scattering_p,
                         const BSDF* phase_func,Sampler& sampler){
        const Sample3 sample = sampler.sample3();

        auto bsdf_sample_ret = phase_func->sample(scattering_p.wo,TransportMode::Radiance,sample);
        if(!bsdf_sample_ret.is_valid())
            return {};
        bsdf_sample_ret.wi = normalize(bsdf_sample_ret.wi);

        Ray ray(scattering_p.pos,bsdf_sample_ret.wi);
        SurfaceIntersection t_isect;
        bool has_intersect = scene.intersect_p(ray,&t_isect);
        if(!has_intersect){
            //sample from environment light
            Spectrum envir_illum;
            if(auto light = scene.environment_light){
                //env light no consider of medium
                Spectrum envir_radiance = light->light_emit(ray.o,ray.d);
                if(!envir_radiance)
                    return {};

                Spectrum f = envir_radiance * bsdf_sample_ret.f;
                if(bsdf_sample_ret.is_delta)
                    envir_illum = f / bsdf_sample_ret.pdf;
                else{
                    auto light_pdf = light->pdf(ray.o,ray.d);
                    auto weight = PowerHeuristic(1,bsdf_sample_ret.pdf,1,light_pdf);
                    envir_illum = f * weight / bsdf_sample_ret.pdf;
                }
            }
            return envir_illum;
        }

        auto light = t_isect.primitive->as_area_light();
        if(!light)
            return {};

        Spectrum light_radiance = light->light_emit(t_isect,t_isect.wo);
        if(!light_radiance)
            return {};

        const auto medium = scattering_p.medium;

        Spectrum f = medium->tr(ray.o,t_isect.pos,sampler) *
                     light_radiance * bsdf_sample_ret.f;

        if(bsdf_sample_ret.is_delta)
            return f / bsdf_sample_ret.pdf;

        real light_pdf = light->pdf(ray.o,t_isect.pos,(Normal3f)t_isect.geometry_coord.z);

        real weight = PowerHeuristic(1,bsdf_sample_ret.pdf,1,light_pdf);

        return f * weight / bsdf_sample_ret.pdf;

    }

Box<Distribution1D> compute_light_power_distribution(const Scene& scene){
    if(scene.lights.empty()) return nullptr;
    std::vector<real> light_power;
    for(const auto& light:scene.lights){
        light_power.emplace_back(light->power().lum());
    }
    return newBox<Distribution1D>(light_power.data(),light_power.size());
}


TRACER_END