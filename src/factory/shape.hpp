//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_FACTORY_SHAPE_HPP
#define TRACER_FACTORY_SHAPE_HPP

#include "core/shape.hpp"
#include "utility/mesh_load.hpp"

TRACER_BEGIN

    std::vector<RC<Shape>> create_triangle_mesh(const mesh_t& mesh);

TRACER_END

#endif //TRACER_FACTORY_SHAPE_HPP
