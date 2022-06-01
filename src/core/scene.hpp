//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_SCENE_HPP
#define TRACER_SCENE_HPP
#include "common.hpp"
#include "core/intersection.hpp"
TRACER_BEGIN

class Scene{
public:
    virtual ~Scene() = default;

    virtual void set_camera(RC<const Camera>) = 0;
    //const RC<Camera>& is not equal to const Camera*
    virtual RC<const Camera> get_camera() const noexcept = 0;

    virtual bool intersect(const Ray& ray) const = 0;

    virtual bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const = 0;

    virtual Bounds3f world_bounds() const noexcept = 0;

    virtual void prepare_to_render() = 0;

    Span<const Light*> lights;

    RC<EnvironmentLight> environment_light;

};

TRACER_END

#endif //TRACER_SCENE_HPP
