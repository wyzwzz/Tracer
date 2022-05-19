//
// Created by wyz on 2022/5/19.
//
#include "core/aggregate.hpp"

TRACER_BEGIN

namespace {
    struct BVHBuildNode;
    struct BVHPrimitiveInfo;

}

    class BVHAccel: public Aggregate{
    public:

        BVHAccel();

        ~BVHAccel() override;

        void build() override;

        void build(const std::vector<RC<Primitive>>& primitives) override;

        bool intersect(const Ray& ray) const noexcept override;

        bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept override;

    };



TRACER_END