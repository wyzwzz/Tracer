//
// Created by wyz on 2022/6/10.
//

#ifndef TRACER_FACTORY_POST_PROCESSOR_HPP
#define TRACER_FACTORY_POST_PROCESSOR_HPP

#include "core/post_processor.hpp"

TRACER_BEGIN

RC<PostProcessor> create_aces_tone_mapper(real exposure);

RC<PostProcessor> create_gamma_corrector(real gamma);

TRACER_END

#endif //TRACER_FACTORY_POST_PROCESSOR_HPP
