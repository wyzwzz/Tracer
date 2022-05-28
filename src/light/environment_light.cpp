//
// Created by wyz on 2022/5/23.
//
#include "core/light.hpp"
#include "core/texture.hpp"
#include "utility/parallel.hpp"
#include "utility/image.hpp"
#include "utility/timer.hpp"
#include "utility/distribution.hpp"
TRACER_BEGIN


    class IBL:public EnvironmentLight{
    private:
        RC<const Texture2D> tex;
        Box<Distribution2D> distrib;
        Spectrum avg_radiance;
    public:
        IBL(const RC<const Texture2D>& tex,const Transform& t):
        tex(tex){
            this->world_to_light = t;
            const int width = tex->width(), height = tex->height();

            for(int y = 0; y < height; ++y){
                const real v0 = static_cast<real>(y) / height;
                const real v1 = static_cast<real>(y + 1) / height;
                for(int x = 0; x < width; ++x){
                    const real u0 = static_cast<real>(x) / width;
                    const real u1 = static_cast<real>(x + 1) / width;
                    const real delta_area = std::abs(2 * PI_r * (u1 - u0) *
                            (std::cos(PI_r * v1) - std::cos(PI_r * v0)));
                    const real u = (u0 + u1) / 2, v = (v0 + v1) / 2;
                    avg_radiance += tex->evaluate({u,v}) * delta_area;
                }
            }


            const int nw = (std::min)(width,360);
            const int nh = (std::min)(height,180);

            AutoTimer timer("pre-process for environment light");

            Image2D<real> img(nw,nh);
            parallel_forrange(0,nh,[&](int,int y){
                const real y0 = static_cast<real>(y) / nh;
                const real y1 = static_cast<real>(y + 1) / nh;

                const real src_y0 = y0 * height;
                const real src_y1 = y1 * height;

                const int src_y_beg = (std::max<int>)(0,std::floor(src_y0) - 1);//确保大于一个像素
                const int src_y_end = (std::min<int>)(height - 1, std::floor(src_y1) + 1);

                for(int x = 0; x < nw; ++x){
                    const real x0 = static_cast<real>(x) / nw;
                    const real x1 = static_cast<real>(x + 1) / nw;

                    const real src_x0 = x0 * width;
                    const real src_x1 = x1 * width;

                    const int src_x_beg = (std::max<int>)(0,std::floor(src_x0) - 1);
                    const int src_x_end = (std::min<int>)(width - 1, std::floor(src_x1) + 1);

                    real pixel_lum = 0;
                    for(int src_y = src_y_beg; src_y <= src_y_end; ++src_y){
                        for(int src_x = src_x_beg; src_x <= src_x_end; ++src_x){
                            const real src_u = static_cast<real>(src_x + 0.5) / width;
                            const real src_v = static_cast<real>(src_y + 0.5) / height;
                            pixel_lum += tex->evaluate({src_u,src_v}).lum();
                        }
                    }

                    const real delta_area = std::abs(
                            2 * PI_r * (x1 - x0) * (std::cos(PI_r * y1 - std::cos(PI_r * y0)))
                            );
                    img.at(x,y) = pixel_lum * delta_area;

                }
            });
            distrib = newBox<Distribution2D>(img.get_raw_data(),nw,nh);
        }

        virtual Spectrum power() const noexcept{
            return avg_radiance * world_radius * world_radius;//todo ?
        }

        LightSampleResult sample_li(const SurfacePoint& ref,const Sample5& sample) const override{
            real map_pdf;
            auto [u,v] = distrib->sample_continuous(sample.u,sample.v,&map_pdf);
            if(map_pdf == 0) return {};
            real theta = v * PI_r;
            real phi = u * 2 * PI_r;
            real cos_theta = std::cos(theta);
            real sin_theta = std::sin(theta);
            real sin_phi = std::sin(phi);
            real cos_phi = std::cos(phi);
            auto light_to_world = inverse(world_to_light);
            Vector3f wi = {sin_theta * cos_phi,sin_theta * sin_phi,cos_theta};
            wi = normalize(light_to_world(wi));
            if(sin_theta == 0)
                return {};
            //g(u,v) = (PI*v,2PI*u) -> (theta,phi)
            //p(theta,phi) = p(u,v) / (2 * PI * PI(
            //p(wi) = p(theta,phi) / sin_theta = p(u,v) / (2 * PI * PI * sin_theta)
            real pdf = map_pdf / ( 2 * PI_r * PI_r * sin_theta);//Jacobian
            LightSampleResult ret;
            ret.ref = ref.pos;
            ret.n = (Normal3f)-wi;
            ret.radiance = light_emit(ref.pos,wi);
            ret.pdf = pdf;
            ret.pos = ref.pos + wi * (2 * world_radius);
            return ret;
        }



        LightEmitResult sample_le(const Sample5&) const override{
            NOT_IMPL
            return {};
        }

        Spectrum light_emit(const Point3f& ref,const Vector3f& ref_to_light) const noexcept override{
            Vector3f dir = normalize(ref_to_light);
            dir = normalize(world_to_light(dir));

            real phi = local_phi(dir);
            real theta = local_theta(dir);
            real u = std::clamp<real>(phi / (2 * PI_r),0,1);
            real v = std::clamp<real>(theta*invPI_r,0,1);
            return tex->evaluate({u,v});
        }

        real pdf(const Point3f& ref, const Vector3f& ref_to_light) const noexcept override{
            Vector3f wi = world_to_light(ref_to_light);
            real theta = spherical_theta(wi),phi = spherical_phi(wi);
            real sin_theta = std::sin(theta);
            if(sin_theta == 0) return 0;
            return distrib->pdf(phi * inv2PI_r,theta * invPI_r) / (2 * PI_r * PI_r * sin_theta);
        }

    };

    RC<EnvironmentLight> create_ibl_light(
            RC<const Texture2D> tex,
            const Transform& t
    ){
        return newRC<IBL>(tex,t);
    }

TRACER_END