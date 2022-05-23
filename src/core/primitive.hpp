//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_PRIMITIVE_HPP
#define TRACER_PRIMITIVE_HPP

#include "utility/geometry.hpp"
#include "common.hpp"
#include "medium.hpp"
TRACER_BEGIN
/**
 * 包装Shape和Material
 */

class AreaLight;

class Primitive{
public:
    virtual ~Primitive() = default;

    virtual bool intersect(const Ray& ray) const noexcept = 0;

    //note ray.t_min and ray.t_max is mutable
    virtual bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept = 0;

    virtual Bounds3f world_bound()  const noexcept = 0;

    virtual const AreaLight* as_area_light() const noexcept = 0;
};



TRACER_END
#endif //TRACER_PRIMITIVE_HPP
