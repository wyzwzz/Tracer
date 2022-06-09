//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_INTERSECTION_HPP
#define TRACER_INTERSECTION_HPP

#include "common.hpp"
#include "medium.hpp"
#include "utility/geometry.hpp"
#include "utility/coordinate.hpp"
TRACER_BEGIN

    struct SurfacePoint{
        Point3f pos;
        Point2f uv;
        Coord geometry_coord;
        Coord shading_coord;

        Point3f eps_offset(const Vector3f& dir) const noexcept{
            if(dot(geometry_coord.z,dir) > 0)
                return pos + geometry_coord.z * eps;
            return pos - geometry_coord.z * eps;
        }
    };

    class SurfaceIntersection: public SurfacePoint{
    public:
        const Primitive* primitive = nullptr;
        const Material*   material = nullptr;
        MediumInterface mi;
        Vector3f wo;

    };

    struct SurfaceShadingPoint{
        const BSDF* bsdf = nullptr;

        Vector3f shading_n;

    };

TRACER_END


#endif //TRACER_INTERSECTION_HPP
