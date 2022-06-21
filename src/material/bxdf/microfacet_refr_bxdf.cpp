//
// Created by wyz on 2022/5/23.
//
#include "microfacet_refr_bxdf.hpp"
#include "../utility/microfacet.hpp"

TRACER_BEGIN

GGXMicrofacetRefractionBXDF::GGXMicrofacetRefractionBXDF(const Spectrum &color, real ior, real roughness,real anisotropic)
:color(color),ior(ior)
{
    const real aspect = anisotropic > 0 ?
                        std::sqrt(1 - real(0.9) * anisotropic) : real(1);
    ax = std::max(real(0.001), roughness * roughness / aspect);
    ay = std::max(real(0.001), roughness * roughness * aspect);
}

Vector3f GGXMicrofacetRefractionBXDF::sample_transmission(const Vector3f &lwo, const Sample2 &sample) const
{
    const Vector3f lwh = microfacet::sample_anisotropic_gtr2(
            ax, ay, sample);
    if(lwh.z <= 0)
        return {};

    if((lwo.z > 0) != (dot(lwh, lwo) > 0))
        return {};

    const real eta = lwo.z > 0 ? 1 / ior : ior;
    const Vector3f owh = dot(lwh, lwo) > 0 ? lwh : -lwh;
    auto opt_lwi = refract_dir(lwo, owh, eta);
    if(!opt_lwi)
        return {};

    const Vector3f lwi = opt_lwi->normalize();
    if(lwi.z * lwo.z > 0 || ((lwi.z > 0) != (dot(lwh, lwi) > 0)))
        return {};

    return lwi;
}

Vector3f GGXMicrofacetRefractionBXDF::sample_inner_reflect(const Vector3f &lwo, const Sample2 &sample) const
{
    assert(lwo.z < 0);

    const Vector3f lwh = microfacet::sample_anisotropic_gtr2(ax, ay, sample);
    if(lwh.z <= 0)
        return {};

    const Vector3f lwi = (2 * dot(lwo, lwh) * lwh - lwo);
    if(lwi.z > 0)
        return {};
    return lwi.normalize();
}

real GGXMicrofacetRefractionBXDF::pdf_transmission(const Vector3f &lwi, const Vector3f &lwo) const
{
    assert(lwi.z * lwo.z < 0);

    const real eta = lwo.z > 0 ? ior : 1 / ior;
    Vector3f lwh = (lwo + eta * lwi).normalize();
    if(lwh.z < 0)
        lwh = -lwh;

    if(((lwo.z > 0) != (dot(lwh, lwo) > 0)) ||
       ((lwi.z > 0) != (dot(lwh, lwi) > 0)))
        return 0;

    const real sdem = dot(lwo, lwh) + eta * dot(lwi, lwh);
    const real dwh_to_dwi = eta * eta * dot(lwi, lwh) / (sdem * sdem);

    const real phi_h       = local_phi(lwh);
    const real sin_phi_h   = std::sin(phi_h);
    const real cos_phi_h   = std::cos(phi_h);
    const real cos_theta_h = lwh.z;
    const real sin_theta_h = std::max<real>(0,std::sqrt(1 - cos_theta_h * cos_theta_h));

    const real D = microfacet::anisotropic_gtr2(
            sin_phi_h, cos_phi_h,
            sin_theta_h, cos_theta_h, ax, ay);
    return std::abs(dot(lwi, lwh) * D * dwh_to_dwi);
}

real GGXMicrofacetRefractionBXDF::pdf_inner_reflect(const Vector3f &lwi, const Vector3f &lwo) const
{
    //todo: same with learnopengl
    assert(lwi.z < 0 && lwo.z < 0);

    const Vector3f lwh = -(lwi + lwo).normalize();
    const real phi_h = local_phi(lwh);
    const real sin_phi_h = std::sin(phi_h);
    const real cos_phi_h = std::cos(phi_h);
    const real cos_theta_h = lwh.z;
    const real sin_theta_h = std::max<real>(0,std::sqrt(1 - cos_theta_h * cos_theta_h));
    const real cos_theta_d = dot(lwi, lwh);

    const real D = microfacet::anisotropic_gtr2(
            sin_phi_h, cos_phi_h,
            sin_theta_h, cos_theta_h, ax, ay);
    return std::abs(cos_theta_h * D / (4 * cos_theta_d));
}

Spectrum GGXMicrofacetRefractionBXDF::evaluate(const Vector3f &lwi, const Vector3f &lwo, TransportMode mode) const
{
    //outside reflect
    if(lwi.z >= 0 && lwo.z >= 0)
        return {};

    //inner reflect
    if(lwi.z < 0 && lwo.z < 0){
        const Vector3f lwh = -(lwi + lwo).normalize();
        assert(lwh.z > 0);

        const real cos_theta_d = dot(lwo, lwh);
        const real F = dielectric_fresnel(ior, 1, cos_theta_d);

        const real phi_h       = local_phi(lwh);
        const real sin_phi_h   = std::sin(phi_h);
        const real cos_phi_h   = std::cos(phi_h);
        const real cos_theta_h = lwh.z;
        const real sin_theta_h = std::max<real>(0,std::sqrt(1 - cos_theta_h * cos_theta_h));
        const real D = microfacet::anisotropic_gtr2(
                sin_phi_h, cos_phi_h,
                sin_theta_h, cos_theta_h,
                ax, ay);

        const real phi_i = local_phi(lwi);
        const real phi_o = local_phi(lwo);
        const real sin_phi_i = std::sin(phi_i);
        const real cos_phi_i = std::cos(phi_i);
        const real sin_phi_o = std::sin(phi_o);
        const real cos_phi_o = std::cos(phi_o);
        const real tan_theta_i = local_tan_theta(lwi);
        const real tan_theta_o = local_tan_theta(lwo);
        const real G = microfacet::smith_anisotropic_gtr2(
                cos_phi_i, sin_phi_i,
                ax, ay, tan_theta_i)
                       * microfacet::smith_anisotropic_gtr2(
                cos_phi_o, sin_phi_o,
                ax, ay, tan_theta_o);

        return Spectrum(std::abs(F * D * G / (4 * lwi.z * lwo.z)));
    }
    //transmission
    const real cos_theta_i = lwi.z;
    const real cos_theta_o = lwo.z;
    const real eta = cos_theta_o > 0 ? ior : 1 / ior;
    Vector3f lwh = (lwo + eta * lwi).normalize();
    if(lwh.z < 0) lwh = -lwh;

    const real cos_theta_d = dot(lwo,lwh);
    const real F = dielectric_fresnel(ior,1,cos_theta_d);

    const real phi_h = local_phi(lwh);
    const real sin_phi_h = std::sin(phi_h);
    const real cos_phi_h = std::cos(phi_h);
    const real cos_theta_h = lwh.z;
    const real sin_theta_h = std::max<real>(0,std::sqrt(1 - cos_theta_h * cos_theta_h));
    const real D = microfacet::anisotropic_gtr2(sin_phi_h,cos_phi_h,sin_theta_h,cos_theta_h,ax,ay);

    const real phi_i = local_phi(lwi);
    const real phi_o = local_phi(lwo);
    const real sin_phi_i = std::sin(phi_i);
    const real cos_phi_i = std::cos(phi_i);
    const real sin_phi_o = std::sin(phi_o);
    const real cos_phi_o = std::cos(phi_o);
    const real tan_theta_i = local_tan_theta(lwi);
    const real tan_theta_o = local_tan_theta(lwo);
    const real Gi = microfacet::smith_anisotropic_gtr2(cos_phi_i,sin_phi_i,ax,ay,tan_theta_i);
    const real Go = microfacet::smith_anisotropic_gtr2(cos_phi_o,sin_phi_o,ax,ay,tan_theta_o);
    const real G = Gi * Go;

    //todo ???
    const real sdem = cos_theta_d + eta * dot(lwi, lwh);
    const real corr_factor = mode == TransportMode::Radiance ? 1 / eta : 1;

    const real val = (1 - F) * D * G * eta * eta
                     * dot(lwi, lwh) * dot(lwo, lwh)
                     * corr_factor * corr_factor
                     / (cos_theta_i * cos_theta_o * sdem * sdem);

    return Spectrum(std::abs(val));
}

real GGXMicrofacetRefractionBXDF::pdf(const Vector3f &lwi, const Vector3f &lwo) const
{
    //outside reflect
    if(lwi.z >= 0 && lwo.z >= 0)
        return 0;

    //from inside to outside transmission
    if(lwo.z > 0)
        return pdf_transmission(lwi,lwo);

    //to inside
    const real macro_F = std::clamp<real>(dielectric_fresnel(ior,1,lwo.z),0.1,0.9);

    if(lwi.z > 0)
        return (1 - macro_F) * pdf_transmission(lwi,lwo);
    return macro_F * pdf_inner_reflect(lwi,lwo);
}

BXDFSampleResult GGXMicrofacetRefractionBXDF::sample(const Vector3f &lwo, TransportMode mode, const Sample2 &sample) const
{
    if(lwo.z > 0){
        //sample transmission
        const Vector3f lwi = sample_transmission(lwo,sample);
        if(!lwi)
            return {};

        BXDFSampleResult ret;
        ret.f = evaluate(lwi,lwo,mode);
        ret.lwi = lwi;
        ret.pdf = pdf(lwi,lwo);
        return ret;
    }
    const real macro_F = std::clamp(
            dielectric_fresnel(ior, 1, lwo.z),
            real(0.1), real(0.9));

    Vector3f lwi;
    if(sample.u >= macro_F){
        const real new_u = (sample.u - macro_F) / (1 - macro_F);
        lwi = sample_transmission(lwo,{new_u,sample.v});
    }
    else{
        const real new_u = sample.u / macro_F;
        lwi = sample_inner_reflect(lwo,{new_u,sample.v});
    }
    if(!lwi)
        return {};

    BXDFSampleResult ret;
    ret.f = evaluate(lwi,lwo,mode);
    ret.lwi = lwi;
    ret.pdf = pdf(lwi,lwo);
    return ret;
}

bool GGXMicrofacetRefractionBXDF::has_diffuse() const
{
    return false;
}

TRACER_END