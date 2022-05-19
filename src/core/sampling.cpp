//
// Created by wyz on 2022/5/19.
//
#include "sampling.hpp"

TRACER_BEGIN

    Point2f ConcentricSampleDisk(const Point2f &u){
        // Map uniform random numbers to $[-1,1]^2$
        Point2f uOffset = 2.f * u - Vector2f(1, 1);

        // Handle degeneracy at the origin
        if (uOffset.x == 0 && uOffset.y == 0) return Point2f(0, 0);

        // Apply concentric mapping to point
        real theta, r;
        if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
            r = uOffset.x;
            theta = PIOver4_r * (uOffset.y / uOffset.x);
        } else {
            r = uOffset.y;
            theta = PIOver2_r - PIOver4_r * (uOffset.x / uOffset.y);
        }
        return r * Point2f(std::cos(theta), std::sin(theta));
    }

TRACER_END
