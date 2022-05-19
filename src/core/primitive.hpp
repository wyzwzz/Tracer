//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_PRIMITIVE_HPP
#define TRACER_PRIMITIVE_HPP

#include "utility/geometry.hpp"
#include "common.hpp"
TRACER_BEGIN
/**
 * 包装Shape和Material
 */
class Primitive{
public:
    virtual ~Primitive() = default;

    virtual bool intersect(const Ray& ray) const noexcept = 0;

    virtual bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept = 0;

    virtual Bounds3f world_bound()  const noexcept = 0;
};

class GeometricPrimitive:public Primitive{
public:
    GeometricPrimitive();

    bool intersect(const Ray& ray) const noexcept override;

    bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept override;

    Bounds3f world_bound()  const noexcept override;
private:
    RC<const Shape> shape;
    RC<const Material> material;
//    MediumInterface medium_inteface;
};

TRACER_END
#endif //TRACER_PRIMITIVE_HPP
