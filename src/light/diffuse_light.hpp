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

    Spectrum light_emit(const Point3f& pos,const Normal3f& n,const Point2f& uv,const Vector3f light_to_out) const noexcept override;

    real pdf(const Point3f& ref,const Point3f& pos,const Normal3f& n) const noexcept override;

    LightSampleResult sample_li(const SurfacePoint& ref,const Sample5&) const override;

    LightEmitResult sample_le(const Sample5&) const override;

    LightEmitPdfResult emit_pdf(const Point3f& ref,const Vector3f& dir,const Vector3f& n) const noexcept override;
private:
    const Spectrum emission;
    const RC<const Shape> shape;
};

TRACER_END
#endif //TRACER_DIFFUSE_LIGHT_HPP
