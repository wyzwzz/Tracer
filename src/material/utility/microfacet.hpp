#pragma once

#include "core/spectrum.hpp"

TRACER_BEGIN

namespace microfacet{

    //helper function
    inline real one_minus_5(real x) noexcept{
        real t = 1 - x;
        real t2 = t * t;
        return t2 * t2 * t;
    }

    //D term
    real anisotropic_gtr2(real sin_phi_h,real cos_phi_h,
                          real sin_theta_h,real cos_theta_h,
                          real ax,real ay) noexcept;

    //D term
    real gtr2(real cos_theta_h,real alpha) noexcept;

    //D term
    real gtr1(real sin_theta_h,real cos_theta_h,real alpha) noexcept;

    //G term
    real smith_anisotropic_gtr2(real cos_phi,real sin_phi,real ax,real ay,real tan_theta) noexcept;

    //G term
    real smith_gtr2(real tan_theta, real alpha) noexcept;

    //sample lwh according to wo and ax ay
    Vector3f sample_anisotropic_gtr2_normal(const Vector3f& lwo,real ax,real ay,const Sample2& sample);

    //sample lwh according to ax ay
    Vector3f sample_anisotropic_gtr2(real ax,real ay,const Sample2& sample) noexcept;

    //sample wh for isotropic with alpha
    Vector3f sample_gtr2(real alpha,const Sample2& sample) noexcept;

    //sample wh for isotropic with alpha
    Vector3f sample_gtr1(real alpha,const Sample2& sample) noexcept;

}


TRACER_END