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
    real pdf = 0;
    bool is_delta = false;
    bool is_valid(){
        return wi && pdf > 0 && !!f;
    }
};

class BSDF{
public:
    BSDF(const BSDF&) = delete;
    BSDF& operator=(const BSDF&) = delete;

    BSDF() = default;

    virtual ~BSDF() = default;

    virtual Spectrum eval(const Vector3f& wi,const Vector3f& wo) const = 0;

    virtual BSDFSampleResult sample(const Vector3f& wo,const Sample3&) const = 0;

    virtual real pdf(const Vector3f& wi, const Vector3f& wo) const = 0;

    virtual bool is_delta() const = 0;

    //used for photo mapping
    virtual bool has_diffuse() const = 0;
};


TRACER_END

#endif //TRACER_BSDF_HPP
