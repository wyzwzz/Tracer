//
// Created by wyz on 2022/5/28.
//
#include <optional>
#include "fresnel_point.hpp"
#include "core/texture.hpp"
#include "core/bsdf.hpp"
#include "core/material.hpp"
TRACER_BEGIN

class GlassBSDF: public BSDF{
public:

    GlassBSDF(const DielectricFresnelPoint* fresnel_point,
              const Spectrum& color_reflect,
              const Spectrum& color_refract,
              const SurfaceIntersection& isect)
              :fresnel_point(fresnel_point),
              color_reflect(color_reflect),
               color_refract(color_refract),
               ng(isect.n),
               ns(isect.shading.n),
               ss(isect.shading.dpdu),
               ts(isect.shading.dpdv)
              {}

    Spectrum eval(const Vector3f& wi,const Vector3f& wo) const override{
        return {};
    }

    BSDFSampleResult sample(const Vector3f& wo,const Sample3& sample) const override{
        const Vector3f lwo = normalize(Vector3f(dot(wo,ss),dot(wo,ts),dot(wo,ns)));
        const Vector3f nor = lwo.z > 0 ? Vector3f(0,0,1) : Vector3f(0,0,-1);

        const Spectrum fr = fresnel_point->evaluate(lwo.z);
//        if(fr.r == 1 && lwo.z < 0){
//            LOG_INFO("perfect specular reflect happened");
//        }
        if(sample.u < fr.r){
//            LOG_INFO("perform specular reflect");
            //perform specular reflect
            const Vector3f lwi = Vector3f(-lwo.x,-lwo.y,lwo.z);
            Vector3f wi = lwi.x * ss + lwi.y * ts + lwi.z * (Vector3f)ns;
            wi = normalize(wi);
            Spectrum f = color_reflect * (fr / std::abs(lwi.z)); //brdf for fresnel specular
            BSDFSampleResult ret;
            ret.wi = wi;
            ret.f = f;
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

        Vector3f wi = lwi.x * ss + lwi.y * ts + lwi.z * (Vector3f)ns;
        wi = normalize(wi);
        Spectrum f = color_refract * (1 - fr.r) / std::abs(lwi.z);
        BSDFSampleResult ret;
        ret.f = f;
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
private:
    static std::optional<Vector3f> refract_dir(const Vector3f& lwo,const Vector3f& n,real eta){
        const real cos_theta_i = std::abs(lwo.z);
        const real sin_theta_i_2 = (std::max)(real(0), 1 - cos_theta_i * cos_theta_i);
        const real sin_theta_t_2 = eta * eta * sin_theta_i_2;
        if(sin_theta_t_2 >= 1)
            return std::nullopt;
        const real cosThetaT = std::sqrt(1 - sin_theta_t_2);
        return (eta * cos_theta_i - cosThetaT) * n - eta * lwo;
    }

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

            shading_p.bsdf = arena.alloc_object<GlassBSDF>(fresnel_point,color_reflect,color_refract,isect);


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