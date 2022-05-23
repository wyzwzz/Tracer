//
// Created by wyz on 2022/5/23.
//
#include "diffuse_light.hpp"
#include "core/intersection.hpp"
TRACER_BEGIN

DiffuseLight::DiffuseLight(const RC<const Shape> &shape, const Spectrum &emission)
:shape(shape),emission(emission)
{

}

Spectrum DiffuseLight::power() const noexcept {
    return {};
}

Spectrum DiffuseLight::light_emit(const SurfacePoint& sp,const Vector3f& w) const noexcept {
     return dot(sp.n,w) > 0 ? emission : Spectrum (0);
}

real DiffuseLight::pdf() const noexcept {
    return 0;
}




TRACER_END