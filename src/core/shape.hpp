//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_SHAPE_HPP
#define TRACER_SHAPE_HPP
#include "intersection.hpp"
#include "utility/geometry.hpp"
TRACER_BEGIN

class Shape{
public:
    virtual ~Shape() = default;

    virtual bool intersect(const Ray& ray) const noexcept = 0;

    virtual bool intersect_p(const Ray& ray,real* hit_t,SurfaceIntersection* isect) const noexcept = 0;

    virtual Bounds3f world_bound() const noexcept = 0;

    virtual real surface_area() const noexcept = 0;

    virtual SurfacePoint sample(real* pdf,const Sample2& sample) const noexcept = 0;

    virtual SurfacePoint sample(const Point3f& ref,real* pdf,const Sample2& sample) const noexcept = 0;

    virtual real pdf(const Point3f& pos) const noexcept = 0;

    virtual real pdf(const SurfacePoint& p) const noexcept = 0;

    virtual real pdf(const SurfacePoint&ref, const Vector3f& wi) const noexcept = 0;

    virtual real pdf(const Point3f& ref,const Point3f& pos) const noexcept = 0;

};


TRACER_END

#endif //TRACER_SHAPE_HPP
