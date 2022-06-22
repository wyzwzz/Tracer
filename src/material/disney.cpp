#include "core/bsdf.hpp"
#include "core/bssrdf.hpp"
#include "core/material.hpp"
#include "core/texture.hpp"
#include "utility/reflection.hpp"
#include "utility/microfacet.hpp"

TRACER_BEGIN

namespace{



    class DisneyBSDF: public LocalBSDF{
        Spectrum C_;
        Spectrum Ctint_;

        real metallic_;
        real roughness_;
        Spectrum specular_scale_;
        real specular_tint_;
        real anisotropic_;
        real sheen_;
        real sheen_tint_;
        real clearcoat_;

        real transmission_;
        real IOR_; // inner IOR / outer IOR

        real transmission_roughness_;
        real trans_ax_, trans_ay_;

        real ax_, ay_;
        real clearcoat_roughness_;

        static constexpr real SS_TRANS_ROUGH = real(0.01);
        struct SampleWeights
        {
            real diffuse          = real(0.25);
            real specular         = real(0.25);
            real clearcoat        = real(0.25);
            real transmission     = real(0.25);
        } sample_w;

        static Spectrum to_tint(const Spectrum &base_color) noexcept
        {
            const real lum = base_color.lum();
            return lum > 0 ? base_color / lum : Spectrum(1);
        }
        static Spectrum schlick(const Spectrum &R0, real cos_theta) noexcept
        {
            return R0 + (Spectrum(1) - R0) * microfacet::one_minus_5(cos_theta);
        }

        static real schlick(const real &R0, real cos_theta) noexcept
        {
            return R0 + (1 - R0) * microfacet::one_minus_5(cos_theta);
        }

        Spectrum f_transmission(const Vector3f& lwi,const Vector3f& lwo,TransportMode mode) const{
            assert(lwi.z * lwo.z < 0);
            const real cos_theta_i = lwi.z;
            const real cos_theta_o = lwo.z;

            const real eta = cos_theta_o > 0 ? IOR_ : 1 / IOR_;
            auto lwh = (lwo + eta * lwi).normalize();

            if(lwh.z < 0)
                lwh = -lwh;

            const real cos_theta_d = dot(lwo, lwh);
            const real F = dielectric_fresnel(IOR_, 1, cos_theta_d);

            const real phi_h       = local_phi(lwh);
            const real sin_phi_h   = std::sin(phi_h);
            const real cos_phi_h   = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h, cos_phi_h,
                    sin_theta_h, cos_theta_h,
                    trans_ax_, trans_ay_);

            const real phi_i       = local_phi(lwi);
            const real phi_o       = local_phi(lwo);
            const real sin_phi_i   = std::sin(phi_i), cos_phi_i = std::cos(phi_i);
            const real sin_phi_o   = std::sin(phi_o), cos_phi_o = std::cos(phi_o);
            const real tan_theta_i = local_tan_theta(lwi);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(
                    cos_phi_i, sin_phi_i,
                    trans_ax_, trans_ay_, tan_theta_i)
                           * microfacet::smith_anisotropic_gtr2(
                    cos_phi_o, sin_phi_o,
                    trans_ax_, trans_ay_, tan_theta_o);

            const real sdem = cos_theta_d + eta * dot(lwi, lwh);
            const real corr_factor = mode == TransportMode::Radiance ? 1 / eta : 1;

            const Spectrum sqrtC = sqrt(C_);

            const real val = (1 - F) * D * G * eta * eta
                             * dot(lwi, lwh) * dot(lwo, lwh)
                             * corr_factor * corr_factor
                             / (cos_theta_i * cos_theta_o * sdem * sdem);

            const real trans_factor = cos_theta_o > 0 ? transmission_ : 1;
            return (1 - metallic_) * trans_factor * sqrtC * std::abs(val);
        }
        Spectrum f_inner_reflection(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z < 0 && lwo.z < 0);

            const Vector3f lwh = -(lwi + lwo).normalize();
            assert(lwh.z > 0);

            const real cos_theta_d = dot(lwo, lwh);
            const real F = dielectric_fresnel(IOR_, 1, cos_theta_d);

            const real phi_h       = local_phi(lwh);
            const real sin_phi_h   = std::sin(phi_h);
            const real cos_phi_h   = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h, cos_phi_h,
                    sin_theta_h, cos_theta_h,
                    trans_ax_, trans_ay_);

            const real phi_i       = local_phi(lwi);
            const real phi_o       = local_phi(lwo);
            const real sin_phi_i   = std::sin(phi_i);
            const real cos_phi_i   = std::cos(phi_i);
            const real sin_phi_o   = std::sin(phi_o);
            const real cos_phi_o   = std::cos(phi_o);
            const real tan_theta_i = local_tan_theta(lwi);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(
                    cos_phi_i, sin_phi_i,
                    trans_ax_, trans_ay_, tan_theta_i)
                           * microfacet::smith_anisotropic_gtr2(
                    cos_phi_o, sin_phi_o,
                    trans_ax_, trans_ay_, tan_theta_o);

            return transmission_ * C_ * std::abs(F * D * G / (4 * lwi.z * lwo.z));
        }
        Spectrum f_diffuse(real cos_theta_i,real cos_theta_o,real cos_theta_d) const{
            const Spectrum f_lambert = C_ / PI_r;
            const real FL = microfacet::one_minus_5(cos_theta_i);
            const real FV = microfacet::one_minus_5(cos_theta_o);
            const real RR = 2 * roughness_ * cos_theta_d * cos_theta_d;
            const Spectrum F_retro_refl = C_ / PI_r * RR * (FL + FV + FL * FV * (RR - 1));

            return f_lambert * (1 - real(0.5) * FL) * (1 - real(0.5) * FV) + F_retro_refl;
        }
        Spectrum f_sheen(real cos_theta_d) const{
            return 4 * sheen_ * mix(Spectrum(1), Ctint_, sheen_tint_)
                   * microfacet::one_minus_5(cos_theta_d);
        }
        Spectrum f_specular(const Vector3f& lwi, const Vector3f& lwo) const{
            assert(lwi.z > 0 && lwo.z > 0);

            const real cos_theta_i = lwi.z;
            const real cos_theta_o = lwo.z;

            const Vector3f lwh = (lwi + lwo).normalize();
            const real cos_theta_d = dot(lwi, lwh);

            const Spectrum Cspec = mix(
                    mix(Spectrum(1), Ctint_, specular_tint_),
                    C_, metallic_);
            const Spectrum dielectric_fresnel_ = Cspec * dielectric_fresnel(IOR_, 1, cos_theta_d);
            const Spectrum conductor_fresnel = schlick(Cspec, cos_theta_d);
            const Spectrum F = mix(
                    specular_scale_ * dielectric_fresnel_, conductor_fresnel, metallic_);

            const real phi_h       = local_phi(lwh);
            const real sin_phi_h   = std::sin(phi_h);
            const real cos_phi_h   = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h, cos_phi_h, sin_theta_h, cos_theta_h, ax_, ay_);

            const real phi_i       = local_phi(lwi);
            const real phi_o       = local_phi(lwo);
            const real sin_phi_i   = std::sin(phi_i), cos_phi_i = std::cos(phi_i);
            const real sin_phi_o   = std::sin(phi_o), cos_phi_o = std::cos(phi_o);
            const real tan_theta_i = local_tan_theta(lwi);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(
                    cos_phi_i, sin_phi_i, ax_, ay_, tan_theta_i)
                           * microfacet::smith_anisotropic_gtr2(
                    cos_phi_o, sin_phi_o, ax_, ay_, tan_theta_o);

            return F * D * G / std::abs(4 * cos_theta_i * cos_theta_o);
        }
        Spectrum f_clearcoat(
                real cos_theta_i, real cos_theta_o,
                real tan_theta_i, real tan_theta_o,
                real sin_theta_h, real cos_theta_h, real cos_theta_d) const{
            assert(cos_theta_i > 0 && cos_theta_o > 0);
            const real D = microfacet::gtr1(
                    sin_theta_h, cos_theta_h, clearcoat_roughness_);
            const real F = schlick(real(0.04), cos_theta_d);
            const real G = microfacet::smith_gtr2(tan_theta_i, real(0.25))
                           * microfacet::smith_gtr2(tan_theta_o, real(0.25));
            return Spectrum(clearcoat_ * D * F * G / std::abs(4 * cos_theta_i * cos_theta_o));
        }
        real pdf_transmission(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z * lwo.z < 0);

            const real eta = lwo.z > 0 ? IOR_ : 1 / IOR_;
            Vector3f lwh = (lwo + eta * lwi).normalize();
            if(lwh.z < 0)
                lwh = -lwh;

            if(((lwo.z > 0) != (dot(lwh, lwo) > 0)) ||
               ((lwi.z > 0) != (dot(lwh, lwi) > 0)))
                return 0;

            const real sdem = dot(lwo, lwh) + eta * dot(lwi, lwh);
            const real dwh_to_dwi = eta * eta * dot(lwi, lwh) / (sdem * sdem);

            const real phi_h       = local_phi(lwh);
            const real sin_phi_h   = std::sin(phi_h);
            const real cos_phi_h   = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);

            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h, cos_phi_h,
                    sin_theta_h, cos_theta_h, trans_ax_, trans_ay_);
            return std::abs(dot(lwi, lwh) * D * dwh_to_dwi);
        }
        real pdf_inner_reflection(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z < 0 && lwo.z < 0);

            const Vector3f lwh = -(lwi + lwo).normalize();
            const real phi_h       = local_phi(lwh);
            const real sin_phi_h   = std::sin(phi_h);
            const real cos_phi_h   = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real cos_theta_d = dot(lwi, lwh);

            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h, cos_phi_h,
                    sin_theta_h, cos_theta_h, trans_ax_, trans_ay_);
            return std::abs(cos_theta_h * D / (4 * cos_theta_d));
        }
        real pdf_diffuse(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z > 0 && lwo.z > 0);
            return CosineHemispherePdf(lwi.z);
        }
        auto pdf_specular_clearcoat(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z > 0 && lwo.z > 0);

            const Vector3f lwh = (lwi + lwo).normalize();
            const real phi_h       = local_phi(lwh);
            const real sin_phi_h   = std::sin(phi_h);
            const real cos_phi_h   = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real cos_theta_d = dot(lwi, lwh);

            const real cos_phi_o = std::cos(local_phi(lwo));
            const real sin_phi_o = local_cos_to_sin(cos_phi_o);

            const real tan_theta_o = local_tan_theta(lwo);

            const real specular_D = microfacet::anisotropic_gtr2(
                    sin_phi_h, cos_phi_h, sin_theta_h, cos_theta_h, ax_, ay_);

            const real pdf_specular =
                    microfacet::smith_anisotropic_gtr2(cos_phi_o, sin_phi_o, ax_, ay_, tan_theta_o)
                    * specular_D / (4 * lwo.z);

            const real clearcoat_D = microfacet::gtr1(
                    sin_theta_h, cos_theta_h, clearcoat_roughness_);
            const real pdf_clearcoat = cos_theta_h * clearcoat_D / (4 * cos_theta_d);

            return std::pair{ pdf_specular, pdf_clearcoat };
        }
        Vector3f sample_transmission(const Vector3f& lwo,const Sample2& sample) const{
            const Vector3f lwh = microfacet::sample_anisotropic_gtr2(
                    trans_ax_, trans_ay_, sample);
            if(lwh.z <= 0)
                return {};

            if((lwo.z > 0) != (dot(lwh, lwo) > 0))
                return {};

            const real eta = lwo.z > 0 ? 1 / IOR_ : IOR_;
            const Vector3f owh = dot(lwh, lwo) > 0 ? lwh : -lwh;
            auto opt_lwi = refract_dir(lwo, owh, eta);
            if(!opt_lwi)
                return {};

            const Vector3f lwi = opt_lwi->normalize();
            if(lwi.z * lwo.z > 0 || ((lwi.z > 0) != (dot(lwh, lwi) > 0)))
                return {};

            return lwi;
        }
        Vector3f sample_inner_reflection(const Vector3f& lwo,const Sample2& sample) const{
            assert(lwo.z < 0);

            const Vector3f lwh = microfacet::sample_anisotropic_gtr2(
                    trans_ax_, trans_ay_, sample);
            if(lwh.z <= 0)
                return {};

            const Vector3f lwi = (2 * dot(lwo, lwh) * lwh - lwo);
            if(lwi.z > 0)
                return {};
            return lwi.normalize();
        }
        Vector3f sample_diffuse(const Sample2& sample) const{
            return CosineSampleHemisphere(sample);
        }
        Vector3f sample_specular(const Vector3f& lwo,const Sample2& sample) const{
            const Vector3f lwh = microfacet::sample_anisotropic_gtr2_with_visible_normal(
                    lwo, ax_, ay_, sample).normalize();
            if(lwh.z <= 0)
                return {};

            const Vector3f lwi = (2 * dot(lwo, lwh) * lwh - lwo).normalize();
            if(lwi.z <= 0)
                return {};

            return lwi;
        }
        Vector3f sample_clearcoat(const Vector3f& lwo,const Sample2& sample) const{
            const Vector3f lwh = microfacet::sample_gtr1(clearcoat_roughness_, sample);
            if(lwh.z <= 0)
                return {};

            const Vector3f lwi = (2 * dot(lwo, lwh) * lwh - lwo).normalize();
            if(lwi.z <= 0)
                return {};

            return lwi;
        }

    public:
        DisneyBSDF(const Coord &geometry_coord, const Coord &shading_coord,
                   const Spectrum &base_color,
                   real metallic,
                   real roughness,
                   const Spectrum &specular_scale,
                   real specular_tint,
                   real anisotropic,
                   real sheen,
                   real sheen_tint,
                   real clearcoat,
                   real clearcoat_gloss,
                   real transmission,
                   real transmission_roughness,
                   real IOR)
                : LocalBSDF(geometry_coord, shading_coord)
        {
            C_     = base_color;
            Ctint_ = to_tint(base_color);

            metallic_       = metallic;
            roughness_      = roughness;
            specular_scale_ = specular_scale;
            specular_tint_  = specular_tint;
            anisotropic_    = anisotropic;
            sheen_          = sheen;
            sheen_tint_     = sheen_tint;

            transmission_  = transmission;
            transmission_roughness_ = transmission_roughness;
            IOR_           = (std::max)(real(1.01), IOR);

            const real aspect = anisotropic > 0 ?
                                std::sqrt(1 - real(0.9) * anisotropic) : real(1);
            ax_ = std::max(real(0.001), square(roughness) / aspect);
            ay_ = std::max(real(0.001), square(roughness) * aspect);

            trans_ax_ = (std::max)(real(0.001), square(transmission_roughness) / aspect);
            trans_ay_ = (std::max)(real(0.001), square(transmission_roughness) * aspect);

            clearcoat_ = clearcoat;
            clearcoat_roughness_ = mix(real(0.1), real(0), clearcoat_gloss);
            clearcoat_roughness_ *= clearcoat_roughness_;
            clearcoat_roughness_ = (std::max)(clearcoat_roughness_, real(0.0001));

            const real A = (std::clamp)(
                    base_color.lum() * (1 - metallic_), real(0.3), real(0.7));
            const real B = 1 - A;

            sample_w.diffuse      = A * (1 - transmission_);
            sample_w.transmission = A * transmission_;
            sample_w.specular     = B * 2 / (2 + clearcoat_);
            sample_w.clearcoat    = B * clearcoat_ / (2 + clearcoat_);
        }

        Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
            if(cause_black_fringes(wi,wo))
                return evaluate_black_fringes(wi,wo);

            const Vector3f lwi = shading_coord.global_to_local(wi).normalize();
            const Vector3f lwo = shading_coord.global_to_local(wo).normalize();

            if(std::abs(lwi.z) < eps || std::abs(lwo.z) < eps)
                return {};
            //transmission
            if(lwi.z * lwo.z < 0){
                if(transmission_ == 0)
                    return {};
                return f_transmission(lwi,lwo,mode) * normal_correct_factor(geometry_coord,shading_coord,wi);
            }
            //inner reflection
            else if(lwi.z < 0 && lwo.z < 0){
                if(transmission_ == 0)
                    return {};
                return f_inner_reflection(lwi,lwo) * normal_correct_factor(geometry_coord,shading_coord,wi);
            }
            //outside surface reflection
            else{
                assert(lwi.z && lwo.z);

                real cos_theta_i = lwi.z;
                real cos_theta_o = lwo.z;

                const Vector3f lwh = (lwi + lwo).normalize();
                real cos_theta_d = dot(lwi,lwh);

                Spectrum diffuse,sheen;
                if(metallic_ < 1){
                    //has diffuse
                    diffuse = f_diffuse(cos_theta_i,cos_theta_o,cos_theta_d);
                    if(sheen_ > 0)
                        sheen = f_sheen(cos_theta_d);
                }
                Spectrum specular = f_specular(lwi,lwo);
                Spectrum clearcoat;
                if(clearcoat_ > 0){
                    const real tan_theta_i = local_tan_theta(lwi);
                    const real tan_theta_o = local_tan_theta(lwo);
                    const real cos_theta_h = local_tan_theta(lwh);
                    const real sin_theta_h = local_cos_to_sin(cos_theta_h);

                    clearcoat = f_clearcoat(
                            cos_theta_i, cos_theta_o, tan_theta_i, tan_theta_o,
                            sin_theta_h, cos_theta_h, cos_theta_d);
                }
                Spectrum ret = (1 - metallic_) * (1 - transmission_) * (diffuse + sheen) + specular + clearcoat;

                return ret * normal_correct_factor(geometry_coord,shading_coord,wi);
            }
        }

        BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sample) const override{
            if(cause_black_fringes(wo))
                return sample_black_fringes(wo, mode, sample);

            const Vector3f lwo = shading_coord.global_to_local(wo).normalize();
            if(std::abs(lwo.z) < eps)
                return {};
            //outside to inside transmission and inner reflection
            if(lwo.z < 0){
                if(transmission_ == 0)
                    return {};


            }
            //inside to outside transmission and outside surface reflection
            real sam_selector = sample.u;
            const Sample2 new_sam{ sample.v, sample.w };

            Vector3f lwi;

            if(sam_selector < sample_w.diffuse)
                lwi = sample_diffuse(new_sam);
            else
                sam_selector -= sample_w.diffuse;

            if(sam_selector < sample_w.transmission)
                lwi = sample_transmission(lwo, new_sam);
            else if(sam_selector -= sample_w.transmission;
                    sam_selector < sample_w.specular)
                lwi = sample_specular(lwo, new_sam);
            else
                lwi = sample_clearcoat(lwo, new_sam);

            if(!lwi)
                return {};

            const Vector3f wi    = shading_coord.local_to_global(lwi);
            const Spectrum f = eval(wi, wo, mode);
            const real pdf    = this->pdf(wi, wo);
            return BSDFSampleResult{wi,f,pdf,false};
        }

        real pdf(const Vector3f& wi, const Vector3f& wo) const override{
            if(cause_black_fringes(wi, wo))
                return pdf_black_fringes(wi, wo);

            const Vector3f lwi = shading_coord.global_to_local(wi).normalize();
            const Vector3f lwo = shading_coord.global_to_local(wo).normalize();

            if(std::abs(lwi.z) < eps || std::abs(lwo.z) < eps)
                return 0;

            //outside to inside transmission and inner reflection
            if(lwo.z < 0){
                if(transmission_ == 0)
                    return 0;
                real marco_F = dielectric_fresnel(IOR_,1,lwo.z);
                marco_F = std::clamp<real>(marco_F,0.1,0.9);
                if(lwi.z > 0)
                    return (1 - marco_F) * pdf_transmission(lwi,lwo);
                else
                    return marco_F * pdf_inner_reflection(lwi,lwo);
            }
            //inside to outside transmission
            if(lwi.z < 0){
                //todo ???
                return sample_w.transmission * pdf_transmission(lwi,lwo);
            }
            //outside surface reflection
            //todo ???
            const real diffuse = pdf_diffuse(lwi, lwo);

            auto [specular, clearcoat] = pdf_specular_clearcoat(lwi, lwo);

            return sample_w.diffuse   * diffuse
                   + sample_w.specular  * specular
                   + sample_w.clearcoat * clearcoat;
        }

        bool is_delta() const override{
            return false;
        }

        bool has_diffuse() const override{
            //not complete metal or totally transmission
            return metallic_ < 1 && transmission_ < 1;
        }

        Spectrum get_albedo() const override{
            return C_;
        }
    };
}

class Disney: public Material{
    RC<const Texture2D> base_color_;
    RC<const Texture2D> metallic_;
    RC<const Texture2D> roughness_;
    RC<const Texture2D> specular_scale_;
    RC<const Texture2D> specular_tint_;
    RC<const Texture2D> anisotropic_;
    RC<const Texture2D> sheen_;
    RC<const Texture2D> sheen_tint_;
    RC<const Texture2D> clearcoat_;
    RC<const Texture2D> clearcoat_gloss_;
    RC<const Texture2D> transmission_;
    RC<const Texture2D> transmission_roughness_;
    RC<const Texture2D> IOR_;

    Box<const NormalMapper> normal_mapper_;
    RC<const BSSRDFSurface> bssrdf_;
public:
    Disney(
            RC<const Texture2D> base_color,
            RC<const Texture2D> metallic,
            RC<const Texture2D> roughness,
            RC<const Texture2D> transmission,
            RC<const Texture2D> transmission_roughness,
            RC<const Texture2D> ior,
            RC<const Texture2D> specular_scale,
            RC<const Texture2D> specular_tint,
            RC<const Texture2D> anisotropic,
            RC<const Texture2D> sheen,
            RC<const Texture2D> sheen_tint,
            RC<const Texture2D> clearcoat,
            RC<const Texture2D> clearcoat_gloss,
            Box<const NormalMapper> normal_mapper,
            RC<const BSSRDFSurface> bssrdf)
    {
        base_color_             = base_color;
        metallic_               = metallic;
        roughness_              = roughness;
        transmission_           = transmission;
        transmission_roughness_ = transmission_roughness;
        IOR_                    = ior;
        specular_scale_         = specular_scale;
        specular_tint_          = specular_tint;
        anisotropic_            = anisotropic;
        sheen_                  = sheen;
        sheen_tint_             = sheen_tint;
        clearcoat_              = clearcoat;
        clearcoat_gloss_        = clearcoat_gloss;

        normal_mapper_ = std::move(normal_mapper);
        bssrdf_ = std::move(bssrdf);
    }

    SurfaceShadingPoint shading(const SurfaceIntersection &inct, MemoryArena &arena) const override
    {
        const Point2f uv = inct.uv;
        const Spectrum base_color             = base_color_      ->evaluate(uv);
        const real     metallic               = metallic_        ->evaluate_s(uv);
        const real     roughness              = roughness_       ->evaluate_s(uv);
        const real     transmission           = transmission_    ->evaluate_s(uv);
        const real     transmission_roughness = transmission_roughness_->evaluate_s(uv);
        const real     ior                    = IOR_             ->evaluate_s(uv);
        const Spectrum specular_scale         = specular_scale_  ->evaluate(uv);
        const real     specular_tint          = specular_tint_   ->evaluate_s(uv);
        const real     anisotropic            = anisotropic_     ->evaluate_s(uv);
        const real     sheen                  = sheen_           ->evaluate_s(uv);
        const real     sheen_tint             = sheen_tint_      ->evaluate_s(uv);
        const real     clearcoat              = clearcoat_       ->evaluate_s(uv);
        const real     clearcoat_gloss        = clearcoat_gloss_ ->evaluate_s(uv);

        const Coord shading_coord = normal_mapper_->reorient(uv, inct.shading_coord);
        const BSDF *bsdf = arena.alloc_object<DisneyBSDF>(
                inct.geometry_coord, shading_coord,
                base_color,
                metallic,
                roughness,
                specular_scale,
                specular_tint,
                anisotropic,
                sheen,
                sheen_tint,
                clearcoat,
                clearcoat_gloss,
                transmission,
                transmission_roughness,
                ior);

        const BSSRDF *bssrdf = bssrdf_->create(inct, arena);

        SurfaceShadingPoint shd = { bsdf, shading_coord.z, bssrdf };
        return shd;
    }
};

RC<Material> create_disney(
        RC<const Texture2D> base_color,
        RC<const Texture2D> metallic,
        RC<const Texture2D> roughness,
        RC<const Texture2D> transmission,
        RC<const Texture2D> transmission_roughness,
        RC<const Texture2D> ior,
        RC<const Texture2D> specular_scale,
        RC<const Texture2D> specular_tint,
        RC<const Texture2D> anisotropic,
        RC<const Texture2D> sheen,
        RC<const Texture2D> sheen_tint,
        RC<const Texture2D> clearcoat,
        RC<const Texture2D> clearcoat_gloss,
        Box<const NormalMapper> normal_mapper,
        RC<const BSSRDFSurface> bssrdf){
    return newRC<Disney>(base_color,
                         metallic,
                         roughness,
                         transmission,
                         transmission_roughness,
                         ior,
                         specular_scale,
                         specular_tint,
                         anisotropic,
                         sheen,
                         sheen_tint,
                         clearcoat,
                         clearcoat_gloss,
                         std::move(normal_mapper),
                         std::move(bssrdf));
}

TRACER_END
