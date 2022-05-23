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
        Normal3f n;
        MediumInterface mi;

        Point3f eps_offset(const Vector3f& dir) const noexcept{
            if(dot(n,dir) > 0)
                return pos + (Vector3f)n * eps;
            return pos - (Vector3f)n * eps;
        }
    };

    class SurfaceIntersection:public SurfacePoint{
    public:
        const Primitive* primitive = nullptr;
        const Material* material = nullptr;

        Vector3f wo;

        Vector3f dpdu,dpdv;
        Vector3f dndu,dndv;
        Normal3f map_n;

    };

    struct SurfaceShadingPoint{
        const BSDF* bsdf = nullptr;

    };

TRACER_END


#endif //TRACER_INTERSECTION_HPP
