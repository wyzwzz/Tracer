//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_FACTORY_FILTER_HPP
#define TRACER_FACTORY_FILTER_HPP

#include "common.hpp"

TRACER_BEGIN

RC<Filter> create_gaussin_filter(real radius,real alpha);

TRACER_END

#endif //TRACER_FACTORY_FILTER_HPP
