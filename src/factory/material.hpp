//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_FACTORY_MATERIAL_HPP
#define TRACER_FACTORY_MATERIAL_HPP


#include "core/material.hpp"
#include "core/texture.hpp"
TRACER_BEGIN

RC<Material> create_phong_material(
        RC<const Texture2D> map_d;

        );

TRACER_END

#endif //TRACER_FACTORY_MATERIAL_HPP
