//
// Created by wyz on 2022/5/19.
//

#ifndef TRACER_SAMPLING_HPP
#define TRACER_SAMPLING_HPP
#include "common.hpp"
#include "utility/geometry.hpp"

TRACER_BEGIN

    template<typename F>
    std::pair<Point3<F>, F> uniform_on_cone(
            F max_cos_theta, F u1, F u2) noexcept
    {
        const F cos_theta = (1 - u1) + u1 * max_cos_theta;
        const F sin_theta = std::sqrt((std::max)(F(0), 1 - cos_theta * cos_theta));
        const F phi = 2 * PI<F> * u2;
        const F pdf = 1 / (2 * PI<F> * (1 - max_cos_theta));
        return {
    { std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta },pdf};
    }

        template<typename F>
        std::pair<Point3<F>, F> uniform_on_sphere(F u1, F u2) noexcept
    {
        static_assert(std::is_floating_point_v<F>, "");

        const F z = 1 - 2 * u1;
        const F phi = 2 * PI<F> * u2;
        const F r = std::sqrt((std::max)(F(0), 1 - z * z));
        const F x = r * std::cos(phi);
        const F y = r * std::sin(phi);
        return { { x, y, z }, inv4PI<F> };
    }

    Point2f ConcentricSampleDisk(const Sample2 &u);

    Vector3f CosineSampleHemisphere(const Sample2& u);

    inline real CosineHemispherePdf(real cos_theta){ return cos_theta * invPI_r; }

    inline real PowerHeuristic(int nf, real fPdf, int ng, real gPdf) {
        real f = nf * fPdf, g = ng * gPdf;
        return (f * f) / (f * f + g * g);
    }

    inline Point2f UniformSampleTriangle(const Sample2& u){
        real su0 = std::sqrt(u.u);
        return Point2f(1-su0,u.v*su0);
    }

    template<typename F, typename I>
    std::pair<I, F> extract_uniform_int(F u, I begin, I end)
    {
        assert(begin < end);

        const I delta = end - begin;
        const I integer = begin + (std::min<I>)(I(u * delta), delta - 1);
        const F real = (std::min<F>)(begin + u * delta - integer, 1);
        return { integer, real };
    }

    template<typename F>
    F sample_inv_cdf_table(
            F u, const F *inv_cdf, size_t tab_size) noexcept
    {
        assert(tab_size >= 2);
        const F global = u * (tab_size - 1);
        const size_t low = (std::min<size_t>)(
                static_cast<size_t>(global), tab_size - 2);
        const F local = global - low;
        return inv_cdf[low] * (1 - local) + inv_cdf[low + 1] * local;
    }

    template<typename F>
    F uniform_on_cone_pdf(F max_cos_theta) noexcept
    {
        return 1 / (2 * PI<F> * (1 - max_cos_theta));
    }
TRACER_END

#endif //TRACER_SAMPLING_HPP
