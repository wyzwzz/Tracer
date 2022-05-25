//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_REFLECTION_HPP
#define TRACER_REFLECTION_HPP

#include "geometry.hpp"

TRACER_BEGIN

inline Vector3f reflect(const Vector3f& w,const Vector3f n) noexcept{
    return 2 * dot(w,n) * n - w;
}


TRACER_END

#endif //TRACER_REFLECTION_HPP
