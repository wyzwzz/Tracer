//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_PHONG_SPECULAR_BXDF_HPP
#define TRACER_PHONG_SPECULAR_BXDF_HPP

#include "bxdf.hpp"

TRACER_BEGIN

class PhongSpecularBXDF: public BXDF{
public:
    PhongSpecularBXDF(const Spectrum& specular,real ns);

    Spectrum evaluate(const Vector3f& wi,const Vector3f& wo, TransportMode mode) const override;

    real pdf(const Vector3f& wi,const Vector3f& wo) const override;

    BXDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample2& sample) const override;

    bool has_diffuse() const override;

private:
    Spectrum specular;
    real ns;
};

TRACER_END

#endif //TRACER_PHONG_SPECULAR_BXDF_HPP
