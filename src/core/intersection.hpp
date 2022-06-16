//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_INTERSECTION_HPP
#define TRACER_INTERSECTION_HPP

#include "common.hpp"
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

    static_assert(sizeof(SurfacePoint) == 92,"");

    class SurfaceIntersection: public SurfacePoint{
    public:
        Vector3f wo;
        const Primitive* primitive   = nullptr;
        const Material* material     = nullptr;
        const Medium* medium_inside  = nullptr;
        const Medium* medium_outside = nullptr;

        const Medium* wo_medium() const{
            return dot(wo,geometry_coord.z) >= 0 ? medium_outside : medium_inside;
        }
        const Medium* medium(const Vector3f& d) const noexcept{
            return dot(d,geometry_coord.z) >= 0 ? medium_outside : medium_inside;
        }
    };
    static_assert(sizeof(SurfaceIntersection) == 136,"");

    struct MediumPoint{
        Point3f pos;
    };

    struct MediumScatteringP: MediumPoint{
        Vector3f wo;
        const Medium* medium = nullptr;
        bool invalid() const noexcept{
            return !wo;
        }
    };
    static_assert(sizeof(MediumScatteringP) == 32,"");

    struct SurfaceShadingPoint{
        const BSDF* bsdf = nullptr;

        Vector3f shading_n;

        const BSSRDF* bssrdf = nullptr;
    };

TRACER_END


#endif //TRACER_INTERSECTION_HPP
