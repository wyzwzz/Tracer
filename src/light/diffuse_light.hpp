//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_DIFFUSE_LIGHT_HPP
#define TRACER_DIFFUSE_LIGHT_HPP
#include "core/light.hpp"
TRACER_BEGIN

class DiffuseLight:public AreaLight{
public:
    DiffuseLight(const RC<const Shape>& shape,const Spectrum& emission);

    Spectrum power() const noexcept override;

    Spectrum light_emit(const SurfacePoint& sp,const Vector3f& w) const noexcept override;

    real pdf() const noexcept override;

private:
    const Spectrum emission;
    const RC<const Shape> shape;
};

TRACER_END
#endif //TRACER_DIFFUSE_LIGHT_HPP
