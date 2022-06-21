#pragma once

#include "../utility/fresnel_point.hpp"
#include "bxdf.hpp"

TRACER_BEGIN

class GGXMicrofacetReflectionBXDF : public BXDF{
public:

    GGXMicrofacetReflectionBXDF(const FresnelPoint* fresnel,real roughness,real anisotropic) noexcept;

    Spectrum evaluate(const Vector3f& lwi,const Vector3f& lwo,TransportMode mode) const override;

    real pdf(const Vector3f& lwi,const Vector3f& lwo) const override;

    BXDFSampleResult sample(const Vector3f& lwo,TransportMode mode,const Sample2& sample) const override;

    bool has_diffuse() const override;

private:
    void ggx(const Vector3f& lwi,const Vector3f& lwo,Spectrum* eval,real* pdf) const;

    const FresnelPoint* fresnel;

    real ax;
    real ay;
};

TRACER_END