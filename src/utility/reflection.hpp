//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_REFLECTION_HPP
#define TRACER_REFLECTION_HPP

#include "geometry.hpp"
#include <optional>
TRACER_BEGIN

inline Vector3f reflect(const Vector3f& w,const Vector3f n) noexcept{
    return 2 * dot(w,n) * n - w;
}

    inline real dielectric_fresnel(
            real eta_i, real eta_o, real cos_theta_i) noexcept
    {
        if(cos_theta_i < 0)
        {
            std::swap(eta_i, eta_o);
            cos_theta_i = -cos_theta_i;
        }

        const real sin_theta_i = std::sqrt((std::max)(
                real(0), 1 - cos_theta_i * cos_theta_i));
        const real sin_theta_t = eta_o / eta_i * sin_theta_i;

        if(sin_theta_t >= 1)
            return 1;

        const real cos_theta_t = std::sqrt((std::max)(
                real(0), 1 - sin_theta_t * sin_theta_t));
        const real para = (eta_i * cos_theta_i - eta_o * cos_theta_t)
                          / (eta_i * cos_theta_i + eta_o * cos_theta_t);
        const real perp = (eta_o * cos_theta_i - eta_i * cos_theta_t)
                          / (eta_o * cos_theta_i + eta_i * cos_theta_t);

        return real(0.5) * (para * para + perp * perp);
    }


    inline std::optional<Vector3f> refract_dir(const Vector3f& lwo,const Vector3f& n,real eta){
        const real cos_theta_i = std::abs(lwo.z);
        const real sin_theta_i_2 = (std::max)(real(0), 1 - cos_theta_i * cos_theta_i);
        const real sin_theta_t_2 = eta * eta * sin_theta_i_2;
        if(sin_theta_t_2 >= 1)
            return std::nullopt;
        const real cosThetaT = std::sqrt(1 - sin_theta_t_2);
        return (eta * cos_theta_i - cosThetaT) * n - eta * lwo;
    }

TRACER_END

#endif //TRACER_REFLECTION_HPP
