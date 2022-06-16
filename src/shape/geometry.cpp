//
// Created by wyz on 2022/5/23.
//
#include "core/primitive.hpp"
#include "core/shape.hpp"
#include "light/diffuse_light.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN

    class GeometricPrimitive:public Primitive{
    public:
        GeometricPrimitive(const RC<const Shape>& shape,
                           const RC<const Material>& material,
                           const MediumInterface& mi,
                           //just apply for diffuse area light
                           const Spectrum& emission //todo change emission from Spectrum to Texture2D
        );

        bool intersect(const Ray& ray) const noexcept override;

        bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept override;

        Bounds3f world_bound()  const noexcept override;

        const AreaLight* as_area_light() const noexcept override;
    private:
        RC<const Shape> shape;
        RC<const Material> material;
        MediumInterface medium_interface;
        Box<DiffuseLight> diffuse_light;
    };

    GeometricPrimitive::GeometricPrimitive(const RC<const Shape> &shape,const RC<const Material> &material,
                                           const MediumInterface& mi,
                                           const Spectrum& emission)
            :shape(shape),material(material), medium_interface(mi)
    {
        assert(mi.inside && mi.outside);
        if(!emission.is_back()){
            diffuse_light = newBox<DiffuseLight>(shape,emission);
            LOG_INFO("create diffuse light from shape");
        }
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

        isect->medium_inside = medium_interface.inside.get();
        isect->medium_outside = medium_interface.outside.get();
        return true;
    }

    Bounds3f GeometricPrimitive::world_bound() const noexcept {
        return shape->world_bound();
    }

    const AreaLight *GeometricPrimitive::as_area_light() const noexcept {
        return diffuse_light.get();
    }

    RC<Primitive> create_geometric_primitive(
            const RC<Shape>& shape,const RC<Material>& material,
            const MediumInterface& mi,const Spectrum& emission){

        return newRC<GeometricPrimitive>(shape,material,mi,emission);
    }

TRACER_END