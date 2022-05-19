//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_AGGREGATE_HPP
#define TRACER_AGGREGATE_HPP
#include <vector>
#include "common.hpp"

TRACER_BEGIN
/**
 * 所有Primitive的加速结构
 * intersect和intersect_p会调用primitive的相关同名api
 */
class Aggregate{
public:
    virtual ~Aggregate() = default;

    virtual void build() = 0;

    virtual void build(const std::vector<RC<Primitive>>& primitives) = 0;

    virtual bool intersect(const Ray& ray) const noexcept = 0;

    virtual bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept = 0;
};

TRACER_END

#endif //TRACER_AGGREGATE_HPP
