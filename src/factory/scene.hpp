//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_FACTORY_SCENE_HPP
#define TRACER_FACTORY_SCENE_HPP
#include "common.hpp"
#include "core/scene.hpp"

TRACER_BEGIN


RC<Scene> create_general_scene();

    RC<Scene> create_general_scene(const RC<Aggregate>& accel
//                                   ,const vector<RC<Light>>& lights
                                   );

TRACER_END


#endif //TRACER_FACTORY_SCENE_HPP
