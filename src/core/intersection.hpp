//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_INTERSECTION_HPP
#define TRACER_INTERSECTION_HPP

#include "common.hpp"
#include "medium.hpp"
#include "utility/geometry.hpp"
TRACER_BEGIN

    struct SurfacePoint{
        Point3f pos;
        Point2f uv;
        MediumInterface mi;
    };

    class SurfaceIntersection:public SurfacePoint{
    public:
        const Primitive* primitive = nullptr;
        const Material* material = nullptr;

        Vector3f dpdu,dpdv;
        Normal3f dndu,dndv;

    };

TRACER_END


#endif //TRACER_INTERSECTION_HPP
