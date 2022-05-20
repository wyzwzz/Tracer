//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_FACTORY_ACCELERATOR_HPP
#define TRACER_FACTORY_ACCELERATOR_HPP

#include "core/aggregate.hpp"

TRACER_BEGIN

RC<Aggregate> create_bvh_accel(int max_leaf_primitives);

TRACER_END

#endif //TRACER_FACTORY_ACCELERATOR_HPP
