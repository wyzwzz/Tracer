//
// Created by wyz on 2022/5/26.
//

#ifndef TRACER_FACTORY_LIGHT_HPP
#define TRACER_FACTORY_LIGHT_HPP

#include "core/light.hpp"

TRACER_BEGIN
class Texture2D;
RC<EnvironmentLight> create_ibl_light(
        RC<const Texture2D> tex,
        const Transform& world_to_light
        );


TRACER_END

#endif //TRACER_FACTORY_LIGHT_HPP
