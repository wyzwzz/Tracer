//
// Created by wyz on 2022/5/19.
//
#include "primitive.hpp"
#include "shape.hpp"
TRACER_BEGIN

    GeometricPrimitive::GeometricPrimitive(const RC<const Shape> &shape,const RC<const Material> &material,
                                           const MediumInterface& mi)
    :shape(shape),material(material), medium_interface(mi)
    {

    }

    bool GeometricPrimitive::intersect(const Ray &ray) const noexcept {
        return shape->intersect(ray);
    }

    bool GeometricPrimitive::intersect_p(const Ray &ray, SurfaceIntersection *isect) const noexcept {
        real hit_t = 0.0;
        if(!shape->intersect_p(ray,&hit_t,isect)) return false;
        ray.t_max = hit_t;//update t_max to decide closet intersection

        isect->primitive = this;
        isect->material = material.get();

        isect->mi = medium_interface;

        return true;
    }

    Bounds3f GeometricPrimitive::world_bound() const noexcept {
        return shape->world_bound();
    }

    RC<Primitive> create_geometric_primitive(
            const RC<Shape>& shape,const RC<Material>& material,
            const MediumInterface& mi){

        return newRC<GeometricPrimitive>(shape,material,mi);
    }

TRACER_END






