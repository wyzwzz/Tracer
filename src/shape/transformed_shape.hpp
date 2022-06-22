#pragma once
#include "core/shape.hpp"

TRACER_BEGIN

class TransformedShape: public Shape{
public:
    TransformedShape(const Transform& local_to_world)
    :local_to_world(local_to_world),world_to_local(inverse(local_to_world)){
        update_scale_ratio();
    }
protected:
    void update_scale_ratio() noexcept{
        local_to_world_scale_ratio = local_to_world(Vector3f(1,0,0)).length();
    }

    Ray to_local(const Ray& wr) const noexcept{
        const Point3f local_origin = world_to_local(wr.o);
        const Vector3f local_dir = world_to_local(wr.d);
        return Ray(local_origin,local_dir,wr.t_min,wr.t_max);
    }

    void to_world(SurfacePoint& spt) const noexcept{
        spt.pos = local_to_world(spt.pos);
        spt.geometry_coord = Coord(local_to_world(spt.geometry_coord.x),
                                   local_to_world(spt.geometry_coord.y),
                                   local_to_world(spt.geometry_coord.z));
        spt.shading_coord = Coord(local_to_world(spt.shading_coord.x),
                                   local_to_world(spt.shading_coord.y),
                                   local_to_world(spt.shading_coord.z));
    }

    void to_world(SurfaceIntersection& isect) const noexcept{
        to_world(static_cast<SurfacePoint&>(isect));
        isect.wo = local_to_world(isect.wo);
    }

    Bounds3f to_world(const Bounds3f &local_bounds) const noexcept{
        const auto [low, high] = local_bounds;

        Bounds3f ret;
        Union(ret,local_to_world(low));
        Union(ret,local_to_world(Point3f{ high.x, low.y,  low.z }));
        Union(ret,local_to_world(Point3f{ low.x,  high.y, low.z }));
        Union(ret,local_to_world(Point3f{ low.x,  low.y,  high.z }));
        Union(ret,local_to_world(Point3f { low.x,  high.y, high.z }));
        Union(ret,local_to_world(Point3f { high.x, low.y,  high.z }));
        Union(ret,local_to_world(Point3f { high.x, high.y, low.z }));
        Union(ret,local_to_world(high));
        return ret;
    }

    Transform local_to_world;
    Transform world_to_local;
    real local_to_world_scale_ratio = 1;

};



TRACER_END