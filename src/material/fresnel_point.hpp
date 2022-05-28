//
// Created by wyz on 2022/5/28.
//

#ifndef TRACER_FRESNEL_POINT_HPP
#define TRACER_FRESNEL_POINT_HPP

#include "utility/reflection.hpp"
#include "core/spectrum.hpp"
TRACER_BEGIN

class FresnelPoint{
public:
    virtual ~FresnelPoint() = default;

    virtual Spectrum evaluate(real cos_theta_i) const noexcept = 0;
};

    class DielectricFresnelPoint:public FresnelPoint{
    public:
        DielectricFresnelPoint(real eta_in,real eta_out) noexcept
        :eta_i(eta_in),eta_o(eta_out)
        {}

        Spectrum evaluate(real cos_theta_i) const noexcept override{
            return Spectrum(dielectric_fresnel(eta_i,eta_o,cos_theta_i));
        }

        real eta_in() const noexcept {return eta_i;}
        real eta_out() const noexcept {return eta_o;}
    private:
        real eta_i;
        real eta_o;
    };



TRACER_END

#endif //TRACER_FRESNEL_POINT_HPP
