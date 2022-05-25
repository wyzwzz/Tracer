//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_BXDF_HPP
#define TRACER_BXDF_HPP
#include "core/bsdf.hpp"
#include "core/spectrum.hpp"
TRACER_BEGIN
struct BXDFSampleResult{
    Spectrum f;
    Vector3f lwi;
    real pdf = 0;
    bool is_valid() const{
        return pdf > 0;
    }
};
class BXDF{
public:

    virtual ~BXDF() = default;

    //wi wo应该是局部坐标系
    virtual Spectrum evaluate(const Vector3f& wi,const Vector3f& wo) const = 0;

    virtual real pdf(const Vector3f& wi,const Vector3f& wo) const = 0;

    virtual BXDFSampleResult sample(const Vector3f& wo,const Sample2& sample) const = 0;



};
TRACER_END
#endif //TRACER_BXDF_HPP
