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

class ConductorFresnelPoint: public FresnelPoint{
public:
    ConductorFresnelPoint(const Spectrum& eta_outside,const Spectrum& eta_inside,const Spectrum& k) noexcept
    :eta_o(eta_outside),eta_i(eta_inside),k(k)
    {
        eta_2 = eta_i / eta_o;
        eta_2 *= eta_2;

        eta_k_2 = k / eta_outside;
        eta_k_2 *= eta_k_2;
    }
    Spectrum evaluate(real cos_theta_i) const noexcept override{
        if(cos_theta_i <= 0)
            return Spectrum();

        const real cos2 = cos_theta_i * cos_theta_i;
        const real sin2 = (std::max)(real(0), 1 - cos2);

        const Spectrum t0 = eta_2 - eta_k_2 - Spectrum(sin2);
        const Spectrum a2b2 = sqrt((t0 * t0 + real(4) * eta_2 * eta_k_2));
        const Spectrum t1 = a2b2 + Spectrum(cos2);
        const Spectrum a = sqrt((real(0.5) * (a2b2 + t0)));
        const Spectrum t2 = 2 * cos_theta_i * a;
        const Spectrum rs = (t1 - t2) / (t1 + t2);

        const Spectrum t3 = cos2 * a2b2 + Spectrum(sin2 * sin2);
        const Spectrum t4 = t2 * sin2;
        const Spectrum rp = rs * (t3 - t4) / (t3 + t4);

        return real(0.5) * (rp + rs);
    }
private:
    Spectrum eta_o;
    Spectrum eta_i;

    Spectrum k;

    Spectrum eta_2,eta_k_2;
};

//todo remove to microfacet bxdf
class PaintedConductorFresnelPoint: public ConductorFresnelPoint{
public:
    PaintedConductorFresnelPoint(const Spectrum& eta_outside,const Spectrum& eta_inside,const Spectrum& k,const Spectrum& color)
    : ConductorFresnelPoint(eta_outside,eta_inside,k),color(color)
    {}

    Spectrum evaluate(real cos_theta_i) const noexcept override{
        return color * ConductorFresnelPoint::evaluate(cos_theta_i);
    }
private:
    Spectrum color;

};



TRACER_END

#endif //TRACER_FRESNEL_POINT_HPP
