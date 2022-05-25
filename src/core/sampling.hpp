//
// Created by wyz on 2022/5/19.
//

#ifndef TRACER_SAMPLING_HPP
#define TRACER_SAMPLING_HPP
#include "common.hpp"
#include "utility/geometry.hpp"

TRACER_BEGIN



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

TRACER_END

#endif //TRACER_SAMPLING_HPP
