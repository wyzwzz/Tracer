#pragma once
#include "separable.hpp"

TRACER_BEGIN

// See http://graphics.pixar.com/library/ApproxBSSRDF/paper.pdf
// https://zhuanlan.zhihu.com/p/523611327
class NormalizedDiffusionBSSRDF: public SeparableBSSRDF{
public:
    /**
     * @param dmfp diffuse mean free path on surface
     */
    NormalizedDiffusionBSSRDF(const SurfaceIntersection& isect,real eta,const Spectrum& A,const Spectrum dmfp) noexcept;

protected:
    Spectrum eval_r(real distance) const override;

    SampleRResult sample_r(int channel, Sample1 sam) const override;

    real pdf_r(int channel, real distance) const override;

private:
    Spectrum A;
    Spectrum s;
    Spectrum l;
    Spectrum d;
};


TRACER_END