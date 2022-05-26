//
// Created by wyz on 2022/5/23.
//
#include "core/light.hpp"
#include "core/texture.hpp"
TRACER_BEGIN


    class IBL:public EnvironmentLight{
    public:
        virtual Spectrum power() const noexcept = 0;

        LightSampleResult sample_li(const SurfacePoint& ref,const Sample5&) const override{

        }

        LightEmitResult sample_le(const Sample5&) const override{

        }

        Spectrum light_emit(const Point3f& ref,const Vector3f& ref_to_light) const noexcept override{
            Vector3f dir = normalize(ref_to_light);
            real phi = local_phi(dir);
            real theta = local_theta(dir);
            real u = std::clamp<real>(phi / (2 * PI_r),0,1);
            real v = std::clamp<real>(theta*invPI_r,0,1);
            return img_tex->evaluate({u,v});
        }

        real pdf(const Point3f& ref, const Vector3f& ref_to_light) const noexcept override{

        }
    private:
        RC<const Texture2D> img_tex;

    };



TRACER_END