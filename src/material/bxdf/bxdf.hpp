//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_BXDF_HPP
#define TRACER_BXDF_HPP

#include "core/bsdf.hpp"
#include "core/spectrum.hpp"

TRACER_BEGIN

struct BXDFSampleResult{
    Vector3f lwi;
    Spectrum f;
    real pdf = 0;
    bool is_valid() const{
        return pdf > 0;
    }
};

class BXDF: public NoCopy{
public:

    virtual ~BXDF() = default;

    virtual Spectrum evaluate(const Vector3f& lwi,const Vector3f& lwo,TransportMode mode) const = 0;

    virtual real pdf(const Vector3f& lwi,const Vector3f& lwo) const = 0;

    virtual BXDFSampleResult sample(const Vector3f& lwo,TransportMode mode,const Sample2& sample) const = 0;

    virtual bool has_diffuse() const = 0;

};

TRACER_END

#endif //TRACER_BXDF_HPP
