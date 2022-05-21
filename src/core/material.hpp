//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_MATERIAL_HPP
#define TRACER_MATERIAL_HPP

#include "utility/geometry.hpp"

TRACER_BEGIN

class Material{
public:
    virtual ~Material() = default;

    virtual Spectrum evaluate(const Point2f& uv) const  = 0;
};

TRACER_END

#endif //TRACER_MATERIAL_HPP
