//
// Created by wyz on 2022/5/19.
//
#include "sampling.hpp"
#include "utility/geometry.hpp"
TRACER_BEGIN

    Point2f ConcentricSampleDisk(const Sample2 &u){
        Point2f u_offset = Point2f(u.u*2,u.v*2) - Vector2f(1,1);
        if(u_offset.x == 0 && u_offset.y == 0) return Point2f(0,0);

        real theta,r;
        if(std::abs(u_offset.x) > std::abs(u_offset.y)){
            r = u_offset.x;
            theta = PIOver4_r * (u_offset.y / u_offset.x);
        }
        else{
            r = u_offset.y;
            theta = PIOver2_r - PIOver4_r * (u_offset.x / u_offset.y);
        }
        return r * Point2f(std::cos(theta),std::sin(theta));
    }

    Vector3f CosineSampleHemisphere(const Sample2& u){
        Point2f d = ConcentricSampleDisk(u);
        real z = std::sqrt(std::max<real>(0,1-d.x*d.x-d.y*d.y));
        return Vector3f(d.x,d.y,z);
    }

TRACER_END
