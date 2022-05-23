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

class Light{
public:
    virtual ~Light() = default;

    virtual Spectrum power() const noexcept = 0;

    virtual const AreaLight* as_area_light() const noexcept {return nullptr;}

    virtual const EnvironmentLight* as_environment_light() const noexcept {return nullptr;}


};

    class AreaLight:public Light{
    public:
        const AreaLight* as_area_light() const noexcept override final {return this;}

        virtual Spectrum light_emit(const SurfacePoint& sp,const Vector3f& w) const noexcept = 0;

        virtual real pdf() const noexcept = 0;
    };

    class EnvironmentLight:public Light{
    public:
        const EnvironmentLight* as_environment_light() const noexcept override final{return this;}

        virtual Spectrum light_emit() const noexcept = 0;

    };

TRACER_END
#endif //TRACER_LIGHT_HPP
