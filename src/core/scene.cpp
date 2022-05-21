//
// Created by wyz on 2022/5/19.
//
#include "core/scene.hpp"
#include "factory/scene.hpp"
#include "core/aggregate.hpp"
TRACER_BEGIN

    class GeneralScene:public Scene{
    private:
    RC<const Camera> scene_camera;
    RC<Aggregate> accel;
    public:
        ~GeneralScene() override {}

        GeneralScene() = default;

        GeneralScene(const RC<Aggregate>& accel)
        :accel(accel)
        {

        }

        void set_camera(RC<const Camera> cam) override{
            scene_camera = cam;
        }

        RC<const Camera> get_camera() const noexcept override{
            return scene_camera;
        }

        bool intersect(const Ray& ray) const override{
            return accel->intersect(ray);
        }

        bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const override{
            return accel->intersect_p(ray,isect);
        }
    };

    RC<Scene> create_general_scene(){
        return newRC<GeneralScene>();
    }

    RC<Scene> create_general_scene(const RC<Aggregate>& accel
//                                   ,const vector<RC<Light>>& lights
    ){
        return newRC<GeneralScene>(accel);
    }

TRACER_END