//
// Created by wyz on 2022/5/19.
//
#include "core/scene.hpp"
#include "factory/scene.hpp"
#include "core/aggregate.hpp"
#include "core/light.hpp"
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

        bool visible(const Point3f& p,const Point3f& q) const override{
            const real dist = (p - q).length();
            Ray r(q,normalize(p - q),eps,dist - eps);
            return !intersect(r);
        }


        void prepare_to_render() override{
            if(environment_light){
                environment_light->preprocess(accel->world_bound());
            }
        }

        Bounds3f world_bounds() const noexcept override{
            return accel->world_bound();
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