//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_BSDF_HPP
#define TRACER_BSDF_HPP

#include "utility/geometry.hpp"
#include "core/spectrum.hpp"
TRACER_BEGIN

struct BSDFSampleResult{
    Vector3f wi;
    Spectrum f;
    real pdf;
    bool is_delta = false;
};

class BSDF{
public:
    BSDF(const BSDF&) = delete;
    BSDF& operator=(const BSDF&) = delete;

    virtual ~BSDF() = default;

    virtual Spectrum eval(const Vector3f& wi,const Vector3f& wo) const = 0;

    virtual BSDFSampleResult sample(const Vector3f& wo,const Sample3&) const = 0;

    virtual real pdf(const Vector3f& wi, const Vector3f& wo) const = 0;
};


TRACER_END

#endif //TRACER_BSDF_HPP
