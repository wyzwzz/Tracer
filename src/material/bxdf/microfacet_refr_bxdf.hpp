//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_MICROFACET_REFR_BXDF_HPP
#define TRACER_MICROFACET_REFR_BXDF_HPP

#include "../utility/fresnel_point.hpp"
#include "bxdf.hpp"

TRACER_BEGIN

class GGXMicrofacetRefractionBXDF: public BXDF{
public:
    GGXMicrofacetRefractionBXDF(const Spectrum& color,real ios,real roughness,real anisotropic);

    Spectrum evaluate(const Vector3f& lwi,const Vector3f& lwo,TransportMode mode) const override;

    real pdf(const Vector3f& lwi,const Vector3f& lwo) const override;

    BXDFSampleResult sample(const Vector3f& lwo,TransportMode mode,const Sample2& sample) const override;

    bool has_diffuse() const override;

private:
    Vector3f sample_transmission(const Vector3f& lwo,const Sample2& sample) const;

    Vector3f sample_inner_reflect(const Vector3f& lwo,const Sample2& sample) const;

    real pdf_transmission(const Vector3f& lwi,const Vector3f& lwo) const;

    real pdf_inner_reflect(const Vector3f& lwi,const Vector3f& lwo) const;

private:
    Spectrum color;
    real ior;
    real ax;
    real ay;
};


TRACER_END
#endif //TRACER_MICROFACET_REFR_BXDF_HPP
