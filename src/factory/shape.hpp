//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_FACTORY_SHAPE_HPP
#define TRACER_FACTORY_SHAPE_HPP

#include "core/shape.hpp"
#include "utility/mesh_load.hpp"

TRACER_BEGIN



RC<Shape> create_sphere(real radius,const Transform& local_to_world);

std::vector<RC<Shape>> create_triangle_mesh(const mesh_t& mesh,const Transform& local_to_world);

TRACER_END

#endif //TRACER_FACTORY_SHAPE_HPP
