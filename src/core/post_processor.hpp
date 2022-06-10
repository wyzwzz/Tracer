//
// Created by wyz on 2022/6/10.
//

#ifndef TRACER__CORE_POST_PROCESSOR_HPP
#define TRACER__CORE_POST_PROCESSOR_HPP

#include "render.hpp"

TRACER_BEGIN

class PostProcessor{
public:
    virtual ~PostProcessor() = default;

    virtual void process(RenderTarget& render_target) = 0;
};

TRACER_END

#endif //TRACER__CORE_POST_PROCESSOR_HPP
