//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_TEXTURE_HPP
#define TRACER_TEXTURE_HPP
#include "utility/geometry.hpp"

TRACER_BEGIN

class Texture2D{
public:
    virtual ~Texture2D() = default;

    virtual int width() const noexcept = 0;

    virtual int height() const noexcept = 0;

    virtual Spectrum evaluate(const SurfaceIntersection& isect) const noexcept = 0;

    virtual Spectrum evaluate(const Point2f& uv) const noexcept = 0;
protected:
    real inv_gamma = 1;
};


TRACER_END

#endif //TRACER_TEXTURE_HPP
