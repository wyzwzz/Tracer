//
// Created by wyz on 2022/6/16.
//

#ifndef TRACER_PHASE_FUNCTION_HPP
#define TRACER_PHASE_FUNCTION_HPP

#include "../core/bsdf.hpp"

TRACER_BEGIN

class HenyeyGreensteinPhaseFunction:public BSDF{
public:
    HenyeyGreensteinPhaseFunction(real g,const Spectrum& albedo)
    :g(g),albedo(albedo)
    {}

    Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
        real u = -cos(wi,wo);
        return Spectrum(phase_func(u));
    }

    BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sam) const override{
        real s = 2 * sam.u - 1;
        real u;
        if(std::abs(g) < real(0.001)){
            u = s;
        }
        else{
            real g2 = g * g;
            u = (1 + g2 - square((1 - g2) / (1 + g * s))) / (2 * g);
        }
        real cos_theta = - u;
        real sin_theta = std::sqrt(std::max<real>(0,1 - cos_theta * cos_theta));
        real phi = 2 * PI_r * sam.v;
        Vector3f local_wi = {
                sin_theta * std::sin(phi),
                sin_theta * std::cos(phi),
                cos_theta
        };
        real phase_value = phase_func(u);
        BSDFSampleResult ret;
        ret.wi = float3_coord::create_from_z(wo).local_to_global(local_wi);
        ret.f = Spectrum(phase_value);
        ret.pdf = phase_value;
        ret.is_delta = false;
        return ret;
    }

    real pdf(const Vector3f& wi, const Vector3f& wo) const override{
        return phase_func(-cos(wi,wo));
    }

    bool is_delta() const override{
        return false;
    }


    bool has_diffuse() const override{
        return true;
    }

    Spectrum get_albedo() const override{
        return albedo;
    }

private:
    //https://www.astro.umd.edu/~jph/HG_note.pdf
    real phase_func(real u) const{
        real g2 = g * g;
        real dem = 1 + g2 - 2 * g * u;
        return (1 - g2) / (4 * PI_r * dem * std::sqrt(dem));
    }
private:
    real g;
    Spectrum albedo;
};



TRACER_END


#endif //TRACER_PHASE_FUNCTION_HPP
