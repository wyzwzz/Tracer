//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_RENDERER_HPP
#define TRACER_RENDERER_HPP

#include "render.hpp"

TRACER_BEGIN

class Renderer{
public:
    virtual ~Renderer(){};

    virtual RenderTarget render(const Scene& scene,Film film) = 0;

};


TRACER_END

#endif //TRACER_RENDERER_HPP
