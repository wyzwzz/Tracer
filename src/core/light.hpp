//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_LIGHT_HPP
#define TRACER_LIGHT_HPP
#include "core/spectrum.hpp"
#include "utility/transform.hpp"
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
    Point3f pos;//emit pos on light
    Vector3f dir;
    Normal3f n;
    Point2f uv;
    Spectrum radiance;
    real pdf_pos;
    real pdf_dir;
    bool is_valid() const{
        return pdf_dir > 0 && pdf_pos > 0 && !radiance.is_back();
    }
};

struct LightEmitPdfResult{
    real pdf_pos;
    real pdf_dir;
};

class Light{
public:
    virtual ~Light() = default;

    virtual Spectrum power() const noexcept = 0;

    virtual const AreaLight* as_area_light() const noexcept {return nullptr;}

    virtual const EnvironmentLight* as_environment_light() const noexcept {return nullptr;}

    virtual LightSampleResult sample_li(const SurfacePoint& ref,const Sample5&) const = 0;

    virtual LightEmitResult sample_le(const Sample5&) const = 0;

    virtual LightEmitPdfResult emit_pdf(const Point3f& ref,const Vector3f& dir,const Vector3f& n) const noexcept = 0;
protected:
    Transform world_to_light;
};

    class AreaLight:public Light{
    public:
        const AreaLight* as_area_light() const noexcept override final {return this;}

        virtual Spectrum light_emit(const SurfacePoint& sp,const Vector3f& w) const noexcept = 0;

        virtual Spectrum light_emit(const Point3f& pos,const Normal3f& n,const Point2f& uv,const Vector3f light_to_out) const noexcept = 0;

        virtual real pdf(
                const Point3f& ref,
                const Point3f& pos,
                const Normal3f& n
                ) const noexcept = 0;
    };

    class EnvironmentLight:public Light{
    protected:
        Point3f world_center;
        real world_radius = 1.f;
    public:
        const EnvironmentLight* as_environment_light() const noexcept override final{return this;}

        virtual Spectrum light_emit(const Point3f& ref,const Vector3f& w) const noexcept = 0;

        virtual real pdf(const Point3f& ref,
                         const Vector3f& ref_to_light) const noexcept = 0;

        void preprocess(const Bounds3f& world_bound) noexcept{
            world_bound.bounding_sphere(&world_center,&world_radius);
        }
    };

TRACER_END
#endif //TRACER_LIGHT_HPP
