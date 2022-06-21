//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_MATERIAL_HPP
#define TRACER_MATERIAL_HPP

#include "utility/geometry.hpp"
#include "utility/memory.hpp"
#include "core/spectrum.hpp"
#include "core/intersection.hpp"
#include "core/texture.hpp"

TRACER_BEGIN


class Material{
public:
    virtual ~Material() = default;

    virtual Spectrum evaluate(const Point2f& uv) const  = 0;

    virtual SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const = 0;

};

class NormalMapper: public NoCopy{
public:
    NormalMapper() = default;

    explicit NormalMapper(RC<const Texture2D> normal_map) noexcept
    :normal_map(normal_map)
    {}

    Coord reorient(const Point2f& uv,const Coord& coord) const noexcept{
        if(!normal_map) return coord;

        Spectrum local_n_ = normal_map->evaluate(uv);
        Vector3f local_n = {
                local_n_.r * 2 - 1,
                local_n_.g * 2 - 1,
                local_n_.b * 2 - 1
        };
        auto world_n = coord.local_to_global(local_n.normalize());
        return coord.rotate_to_new_z(world_n);
    }

private:
    RC<const Texture2D> normal_map;
};

TRACER_END

#endif //TRACER_MATERIAL_HPP
