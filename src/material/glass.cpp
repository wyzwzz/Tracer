//
// Created by wyz on 2022/5/28.
//
#include <optional>
#include "material/utility/fresnel_point.hpp"
#include "core/texture.hpp"
#include "core/bsdf.hpp"
#include "core/material.hpp"
TRACER_BEGIN

class GlassBSDF: public LocalBSDF{
public:

    GlassBSDF(const DielectricFresnelPoint* fresnel_point,
              const Spectrum& color_reflect,
              const Spectrum& color_refract,
              const Coord& geometry_coord,
              const Coord& shading_coord)
              :
              LocalBSDF(geometry_coord,shading_coord),
              fresnel_point(fresnel_point),
              color_reflect(color_reflect),
               color_refract(color_refract)
              {}

    Spectrum eval(const Vector3f& wi,const Vector3f& wo, TransportMode mode) const override{
        return {};
    }

    BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sample) const override{
        const Vector3f lwo = shading_coord.global_to_local(wo).normalize();
        const Vector3f nor = lwo.z > 0 ? Vector3f(0,0,1) : Vector3f(0,0,-1);

        const Spectrum fr = fresnel_point->evaluate(lwo.z);
//        if(fr.r == 1 && lwo.z < 0){
//            LOG_INFO("perfect specular reflect happened");
//        }
        if(sample.u < fr.r){
//            LOG_INFO("perform specular reflect");
            //perform specular reflect
            const Vector3f lwi = Vector3f(-lwo.x,-lwo.y,lwo.z);
            Vector3f wi = shading_coord.local_to_global(lwi);
            Spectrum f = color_reflect * (fr / std::abs(lwi.z)); //brdf for fresnel specular
            real normal_corr = normal_correct_factor(geometry_coord,shading_coord,wi);
            BSDFSampleResult ret;
            ret.wi = wi;
            ret.f = f * normal_corr;
            ret.pdf = fr.r;
            ret.is_delta = true;
            return ret;
        }
//        LOG_INFO("perform specular refract");
        //perform refract
        const real eta_i = lwo.z > 0 ? fresnel_point->eta_out() : fresnel_point->eta_in();
        const real eta_t = lwo.z > 0 ? fresnel_point->eta_in() : fresnel_point->eta_out();
        const real eta = eta_i / eta_t;
        auto opt_lwi = refract_dir(lwo,nor,eta);
        if(!opt_lwi){
            return {};
        }
        Vector3f lwi = normalize(opt_lwi.value());

        Vector3f wi = shading_coord.local_to_global(lwi);

        real corr_factor = mode == TransportMode::Radiance ? eta * eta : 1;

        Spectrum f = corr_factor * color_refract * (1 - fr.r) / std::abs(lwi.z);

        real normal_corr = normal_correct_factor(geometry_coord,shading_coord,wi);

        BSDFSampleResult ret;
        ret.f = f * normal_corr;
        ret.wi = wi;
        ret.pdf = 1 - fr.r;
        ret.is_delta = true;
        return ret;
    }

    real pdf(const Vector3f& wi, const Vector3f& wo) const override{
        return 0;
    }

    bool is_delta() const override{
        return true;
    }

    bool has_diffuse() const override{
        return false;
    }

    Spectrum get_albedo() const override{
        return real(0.5) * (color_refract + color_reflect);
    }
private:


    const DielectricFresnelPoint* fresnel_point;
    Spectrum color_reflect;
    Spectrum color_refract;
    Normal3f ng;
    Normal3f ns;
    Vector3f ss;
    Vector3f ts;
};

    class Glass:public Material{
    public:
        Glass(RC<const Texture2D> map_kr,
              RC<const Texture2D> map_kt,
              RC<const Texture2D> ior)
              :map_kr(map_kr), map_kt(map_kt),map_ior(ior)
              {}

        Spectrum evaluate(const Point2f& uv) const override{
            return map_kr->evaluate(uv);
        }

        SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const override{
            SurfaceShadingPoint shading_p;
            const real ior = map_ior->evaluate_s(isect);

            Spectrum color_reflect = map_kr->evaluate(isect);
            Spectrum color_refract;
            if(map_kr == map_kt){
                color_refract = sqrt(color_reflect);
            }
            else{
                color_refract = map_kt->evaluate(isect);
                color_refract = sqrt(color_refract);//todo ???
            }

            const DielectricFresnelPoint* fresnel_point = arena.alloc_object<DielectricFresnelPoint>(ior,real(1));

            shading_p.bsdf = arena.alloc_object<GlassBSDF>(fresnel_point,color_reflect,color_refract,
                                                           isect.geometry_coord,isect.shading_coord);


            return shading_p;
        }

    private:
        RC<const Texture2D> map_kr;
        RC<const Texture2D> map_kt;
        RC<const Texture2D> map_ior;
    };

    RC<Material> create_glass(
            RC<const Texture2D> map_kr,
            RC<const Texture2D> map_kt,
            RC<const Texture2D> ior
    ){
        return newRC<Glass>(map_kr,map_kt,ior);
    }

TRACER_END