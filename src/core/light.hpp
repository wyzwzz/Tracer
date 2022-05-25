//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_LIGHT_HPP
#define TRACER_LIGHT_HPP
#include "core/spectrum.hpp"

TRACER_BEGIN
struct SurfacePoint;
class AreaLight;
class EnvironmentLight;


struct LightSampleResult{
    Point3f ref;
    Point3f pos;
    Normal3f n;
    Point2f uv;
    Spectrum radiance;
    real pdf;

    bool is_valid() const noexcept{
        return !pdf;
    }

};

struct LightEmitResult{

};

class Light{
public:
    virtual ~Light() = default;

    virtual Spectrum power() const noexcept = 0;

    virtual const AreaLight* as_area_light() const noexcept {return nullptr;}

    virtual const EnvironmentLight* as_environment_light() const noexcept {return nullptr;}

    virtual LightSampleResult sample_li(const SurfacePoint& ref,const Sample5&) const = 0;

    virtual LightEmitResult sample_le(const Sample5&) const = 0;
};

    class AreaLight:public Light{
    public:
        const AreaLight* as_area_light() const noexcept override final {return this;}

        virtual Spectrum light_emit(const SurfacePoint& sp,const Vector3f& w) const noexcept = 0;

        virtual real pdf(
                const Point3f& ref,
                const Point3f& pos,
                const Normal3f& n
                ) const noexcept = 0;
    };

    class EnvironmentLight:public Light{
    public:
        const EnvironmentLight* as_environment_light() const noexcept override final{return this;}

        virtual Spectrum light_emit(const Point3f& ref,const Vector3f& w) const noexcept = 0;

        virtual real pdf(const Point3f& ref,
                         const Vector3f& ref_to_light) const noexcept = 0;

    };

TRACER_END
#endif //TRACER_LIGHT_HPP
