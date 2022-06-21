//
// Created by wyz on 2022/5/23.
//
#include "microfacet_refl_bxdf.hpp"
#include "../utility/microfacet.hpp"
TRACER_BEGIN

GGXMicrofacetReflectionBXDF::GGXMicrofacetReflectionBXDF(const FresnelPoint *fresnel, real roughness,real anisotropic) noexcept
:fresnel(fresnel)
{
    real aspect = anisotropic > 0 ? std::sqrt(1 - real(0.9) * anisotropic) : real(1);
    ax = std::max(real(0.001),roughness * roughness / aspect);
    ay = std::max(real(0.001),roughness * roughness * aspect);
}

void GGXMicrofacetReflectionBXDF::ggx(const Vector3f &lwi, const Vector3f &lwo, Spectrum *eval, real *pdf) const{
    const real cos_theta_i = lwi.z;
    const real cos_theta_o = lwo.z;
    const Vector3f lwh = (lwi + lwo).normalize();
    const real cos_theta_d = dot(lwi,lwh);

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
    if(eval){
        const real G = Gi * Go;
        const Spectrum F = fresnel->evaluate(cos_theta_d);
        *eval = F * D * G / std::abs(4 * cos_theta_i * cos_theta_o);
    }
    if(pdf){
        *pdf = Go * D / (4 * cos_theta_o);
    }
}

Spectrum GGXMicrofacetReflectionBXDF::evaluate(const Vector3f &lwi, const Vector3f &lwo, TransportMode mode) const {
    Spectrum ret;
    if(lwi.z <= 0 || lwo.z <= 0) return ret;
    ggx(lwi,lwo,&ret,nullptr);
    return ret;
}

BXDFSampleResult GGXMicrofacetReflectionBXDF::sample(const Vector3f &lwo, TransportMode mode, const Sample2 &sample) const {
    if(lwo.z <= 0)
        return {};
    const Vector3f lwh = microfacet::sample_anisotropic_gtr2_normal(lwo,ax,ay,sample).normalize();
    if(lwh.z <= 0)
        return {};
    const Vector3f lwi = (2 * dot(lwo,lwh) * lwh - lwo).normalize();
    if(lwi.z <= 0)
        return {};
    BXDFSampleResult ret;
    ret.lwi = lwi;
    ggx(lwi,lwo,&ret.f,&ret.pdf);
    return ret;
}

real GGXMicrofacetReflectionBXDF::pdf(const Vector3f &lwi, const Vector3f &lwo) const {
    real pdf = 0;
    if(lwi.z <= 0 || lwo.z <= 0) return pdf;
    ggx(lwi,lwo,nullptr,&pdf);
    return pdf;
}

bool GGXMicrofacetReflectionBXDF::has_diffuse() const {
    return false;
}

TRACER_END