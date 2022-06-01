//
// Created by wyz on 2022/5/23.
//
#include "phong_specular_bxdf.hpp"
#include "utility/reflection.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN

    Vector3f PowCosineSampleHemisphere(real ns,const Sample2& sample){
        const real cos_theta_h = std::pow(sample.u,1 / (ns + 1));
        const real sin_theta_h = std::sqrt(std::max<real>(0,1-cos_theta_h*cos_theta_h));
        const real phi = 2 * PI_r * sample.v;
        return normalize(Vector3f(
                sin_theta_h * std::cos(phi),
                sin_theta_h * std::sin(phi),
                cos_theta_h
                ));
    }

    real PowCosineSampleHemispherePdf(real e,real cos_theta){
        return (e + 1) * (inv2PI_r) * std::pow(cos_theta,e);
    }

    PhongSpecularBXDF::PhongSpecularBXDF(const Spectrum& specular,real ns)
            :specular(specular),ns(ns)
    {

    }

    Spectrum PhongSpecularBXDF::evaluate(const Vector3f& lwi,const Vector3f& lwo) const{
        if(lwi.z <= 0 || lwo.z <= 0)
            return {};
        const Vector3f ideal_lwi = reflect(lwo,Vector3f(0,0,1));
        const real cos_val = dot(ideal_lwi,lwi)/(lwi.length() * ideal_lwi.length());
        if(cos_val <= 0)
            return{};
        auto ret = specular * PowCosineSampleHemispherePdf(ns,cos_val);
//        LOG_INFO("evaluate from specular: {} {} {}",ret.r,ret.g,ret.b);
        return ret;
    }

    real PhongSpecularBXDF::pdf(const Vector3f& lwi,const Vector3f& lwo) const{
        if(lwi.z <= 0 || lwo.z <= 0)
            return 0;
        const Vector3f ideal_lwi = reflect(lwo,{0,0,1});
        //compute cos theta for ideal_lwi and lwi
        const real cos_theta = dot(lwi,ideal_lwi) / (lwi.length() * ideal_lwi.length());
        if(cos_theta <= 0)
            return 0;
        return PowCosineSampleHemispherePdf(ns,cos_theta);
    }

    BXDFSampleResult PhongSpecularBXDF::sample(const Vector3f& lwo,const Sample2& sample) const{
        //all is in local coord
        if(lwo.z <= 0)
            return {};
        const Vector3f ideal_lwi = normalize(reflect(lwo,{0,0,1}));
        Vector3f local_lwi = PowCosineSampleHemisphere(ns,sample);
//        local_lwi = Vector3f(0,0,1);
        Vector3f x,y;
        coordinate(ideal_lwi,x,y);
        Vector3f lwi = local_lwi.z * ideal_lwi + local_lwi.x * x + local_lwi.y * y;
        if(lwi.z <= 0)
            return {};

        const real pdf = PowCosineSampleHemispherePdf(ns,local_lwi.z);//note!!!
        BXDFSampleResult ret;
        ret.f = evaluate(lwi,lwo);
        ret.lwi = lwi;
        ret.pdf = pdf;
//        LOG_INFO("sample specular pdf: {}, f: {} {} {}, specular: {} {} {}",pdf,ret.f.r,ret.f.g,ret.f.b,specular.r,specular.g,specular.b);
        return ret;
    }

    bool PhongSpecularBXDF::has_diffuse() const {
        return false;
    }

TRACER_END