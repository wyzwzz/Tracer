#pragma once

#include "../../core/bssrdf.hpp"


TRACER_BEGIN

class SeparableBSSRDF: public BSSRDF{
public:
    SeparableBSSRDF(const SurfaceIntersection& po,real eta);

    BSSRDFSampleResult sample_pi(const Scene& scene,const Sample3& sample,MemoryArena& arena) const override;
protected:
    struct SampleRResult{
        Spectrum  coef;
        real dist = 0;
        real pdf  = 0;
    };
    virtual SampleRResult sample_r(int channel,Sample1 sample) const = 0;

    virtual Spectrum eval_r(real distance) const = 0;

    virtual real pdf_r(int channel,real distance) const = 0;

    real eta;
private:
    real pdf_pi(const SurfaceIntersection& pi) const;

    SurfaceIntersection po;

};


TRACER_END