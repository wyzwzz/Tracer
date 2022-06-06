//
// Created by wyz on 2022/5/23.
//
#include "diffuse_light.hpp"
#include "core/intersection.hpp"
#include "core/shape.hpp"
#include "utility/logger.hpp"
#include "core/sampling.hpp"
TRACER_BEGIN

DiffuseLight::DiffuseLight(const RC<const Shape> &shape, const Spectrum &emission)
:shape(shape),emission(emission)
{

}

Spectrum DiffuseLight::power() const noexcept {
    //todo ???
    return PI_r * emission * shape->surface_area();
}

Spectrum DiffuseLight::light_emit(const SurfacePoint& sp,const Vector3f& w) const noexcept {
     return dot(sp.n,w) > 0 ? emission : Spectrum (0);
}

Spectrum DiffuseLight::light_emit(const Point3f &pos, const Normal3f &n, const Point2f &uv,
                                  const Vector3f light_to_out) const noexcept {
    return dot(n,light_to_out) > 0 ? emission : Spectrum(0);
}

real DiffuseLight::pdf(const Point3f& ref,const Point3f& pos,const Normal3f& n) const noexcept {
    if(dot(n,ref - pos) <= 0)
        return 0;
    real area = shape->surface_area();

    const Vector3f p2ref = ref - pos;
    real dist2 = p2ref.length_squared();
    real abscos = std::abs(dot(n,p2ref)/(n.length() * p2ref.length()));
    area *= abscos;
    // (1 / 4*PI) / (A * cos_theta / (4 * PI * R * R) = R * R / ( A * cos_theta)
    return  dist2 / area;
}

LightSampleResult DiffuseLight::sample_li(const SurfacePoint& ref,const Sample5& sample) const {
    real area_pdf;//equal to 1 / surface area
    auto sp = shape->sample(ref,&area_pdf,{sample.u,sample.v});
    if(dot(sp.n,ref.pos - sp.pos) <= 0)
        return {};
    Vector3f sp2ref = ref.pos - sp.pos;
    real dist2 = sp2ref.length_squared();
    real pdf = area_pdf * dist2 / std::abs(dot(sp.n,sp2ref) / (sp2ref.length()));

    LightSampleResult ret;
    ret.pos = sp.pos;
    ret.pdf = pdf;
    ret.radiance = emission;
    ret.n = sp.n;
    ret.uv = sp.uv;
    return ret;
}

LightEmitResult DiffuseLight::sample_le(const Sample5 &sample) const {
    real pdf_pos;
    auto sample_pt = shape->sample(&pdf_pos,{sample.u,sample.v});
    auto local_dir = CosineSampleHemisphere({sample.r,sample.s});
    auto pdf_dir = CosineHemispherePdf(local_dir.z);
    Vector3f s,t;
    coordinate((Vector3f)sample_pt.n,s,t);
    auto world_dir = local_dir.x * s + local_dir.y * t + local_dir.z * (Vector3f)sample_pt.n;

    LightEmitResult ret;
    ret.n = sample_pt.n;
    ret.pos = sample_pt.eps_offset(world_dir);
    ret.uv = sample_pt.uv;
    ret.dir = world_dir;
    ret.pdf_dir = pdf_dir;
    ret.pdf_pos = pdf_pos;
    ret.radiance = emission;

    return ret;
}

LightEmitPdfResult DiffuseLight::emit_pdf(const Point3f &ref, const Vector3f &dir, const Vector3f &n) const noexcept {
    real pdf_pos = shape->pdf(ref);
    real cos_theta = dot(dir,n) / (dir.length() * n.length());
    real pdf_dir = CosineHemispherePdf(cos_theta);
    LightEmitPdfResult ret;
    ret.pdf_pos = pdf_pos;
    ret.pdf_dir = pdf_dir;
    return ret;
}



TRACER_END