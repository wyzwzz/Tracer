#include "core/bsdf.hpp"
#include "core/material.hpp"
#include "core/texture.hpp"
#include "core/bssrdf.hpp"
#include "utility/reflection.hpp"
#include "utility/microfacet.hpp"
#include "factory/material.hpp"
#include "bssrdf/separable.hpp"
TRACER_BEGIN




namespace {

    class DisneyBSSRDF : public SeparableBSSRDF{
    public:
        DisneyBSSRDF(const SurfaceIntersection& isect,real eta,const Spectrum& A,const Spectrum dmfp)
        : SeparableBSSRDF(isect,eta),A(A),l(dmfp)
        {
            s = A.map([](real c){
                const real t = c - real(0.33);
                return real(3.5) + 100 * t * t * t * t;
            });
            inv_s = s.map([](real c){
                return real(1) / c;
            });
            d = l / s;
            l2 = l * l;
        }
    private:
        SampleRResult sample_r(int channel,Sample1 sample) const override{
            const real u = sample.u;
            const real G = 1 + 4 * u * (2 * u + std::sqrt(1 + 4 * square(u)));
            const real sr = 3 * std::log((1 + std::pow<real>(G,-real(1) / 3) + std::pow<real>(G, real(1) / 3)) / (4 * u));
            const auto r = sr * inv_s[channel] * l[channel];

            SampleRResult ret;
            ret.coef = eval_r(r);
            ret.dist = r;
            ret.pdf = pdf_r(channel,r);
            return ret;
        }

        Spectrum eval_r(real distance) const override{
            return A * (exp(-Spectrum(distance) * s) + exp(-Spectrum(distance) * s / real(3)))
                   / (8 * PI_r * distance) / l2;
        }

        real pdf_r(int channel,real distance) const override{
            const real r = distance / l[channel];
            return s[channel] * (std::exp(-s[channel] * r) + std::exp(-s[channel] * r / real(3))) / (8 * PI_r);
        }
        Spectrum A;
        Spectrum s;
        Spectrum inv_s;
        Spectrum l;
        Spectrum d;
        Spectrum l2;
    };

    template<bool Thin>
    class DisneyBSDF: public LocalBSDF{
        Spectrum C;
        Spectrum Ctint; // for sheen

        real metallic;
        real ior;
        real roughness;
        real specular_scale;
        real specular_tint;
        real anisotropic;
        real sheen;
        real sheen_tint;

        real spec_trans;
        real scatter_dist;
        real flatness;
        real diffuse_trans;

        real clearcoat;
        real clearcoat_gloss;

        real clearcoat_roughness;

        real diffuse_weight;

        real ax,ay;//specular and transmission use same roughness alpha

        struct{
            real specular_weight = 0.25;
            real transmission_weight = 0.25;
            real diffuse_weight = 0.25;
            real clearcoat_weight = 0.25;
        }sample_weight;
    public:
        DisneyBSDF(const Coord& geometry_coord,const Coord& shading_coord,
                   const Spectrum& base_color_,
                   real metallic_,
                   real ior_,
                   real roughness_,
                   real specular_,
                   real specular_tint_,
                   real anisotropic_,
                   real sheen_,
                   real sheen_tint_,
                   real clearcoat_,
                   real clearcoat_gloss_,
                   real spec_trans_,
                   real scatter_dist_,
                   real flatness_,
                   real diffuse_trans_)
                   : LocalBSDF(geometry_coord,shading_coord)
        {

            diffuse_weight = (1 - metallic_) * (1 - spec_trans_);


            {
                real specular_w = metallic + diffuse_weight;
                real diffuse_w = diffuse_weight;
                real transmission_w = (1 - metallic) * spec_trans;
                real clearcoat_w = clearcoat;
                real s = 1 / (specular_w + diffuse_w + transmission_w + clearcoat_w);
                sample_weight.specular_weight = specular_w * s;
                sample_weight.diffuse_weight = diffuse_w * s;
                sample_weight.transmission_weight = transmission_w * s;
                sample_weight.clearcoat_weight = clearcoat_w * s;
            }
        }

        Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
            if(cause_black_fringes(wi, wo))
                return evaluate_black_fringes(wi, wo);

            const auto lwi = shading_coord.global_to_local(wi).normalize();
            const auto lwo = shading_coord.global_to_local(wo).normalize();

            if(std::abs(lwi.z) < eps || std::abs(lwo.z) < eps)
                return {};

            // transmission / refract
            if(lwi.z * lwo.z < 0){
                if(!spec_trans) return {};
                const auto value = eval_transmission(lwi,lwo,mode);
                return value * normal_correct_factor(geometry_coord,shading_coord,wi);
            }

            // inner reflect
            if(lwi.z < 0 && lwo.z < 0){
                if(!spec_trans) return {};
                const auto value = eval_inner_reflect(lwi,lwo);
                return value * normal_correct_factor(geometry_coord,shading_coord,wi);
            }

            // reflect
            // specular(specular and clearcoat) + diffuse(diffuse or subsurface and sheen)
            assert(lwi.z >= eps && lwo.z >= eps);

            Spectrum diffuse_v, sheen_v;
            if(diffuse_weight > 0){
                if constexpr(Thin){
                    // blend between disney diffuse and subsurface based on flatness
                    const auto d = eval_diffuse(lwi,lwo);
                    const auto s = eval_subsurface(lwi,lwo);
                    diffuse_v += (d * (1 - flatness) + s * flatness) * (1 - diffuse_trans);
                }
                else{
                    if(!scatter_dist){
                        // no subsurface but just disney diffuse
                        diffuse_v += eval_diffuse(lwi,lwo);
                    }
                    else{
                        // this part would be handled with bssrdf

                    }
                }
                // retro-reflect
                diffuse_v += eval_retro_reflect(lwi,lwo);

                if(sheen > 0){
                    sheen_v = eval_sheen(lwi,lwo);
                }

            }

            Spectrum specular_v = eval_specular(lwi,lwo);
            Spectrum clearcoat_v;
            if(clearcoat > 0){
                clearcoat_v = eval_clearcoat(lwi,lwo);
            }

            const auto value = diffuse_weight * (diffuse_v + sheen_v) + specular_v + clearcoat_v;

            return value * normal_correct_factor(geometry_coord,shading_coord,wi);
        }

        BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sample) const override{


            const auto lwo = shading_coord.global_to_local(wo).normalize();

            if(lwo.z < 0){
                // transmission + inner reflect
                if(!spec_trans) return {};

                Vector3f lwi;
                real sel = sample.u;
                const Sample2 new_sam{sample.v,sample.w};

                real macro_F = dielectric_fresnel(ior,1,lwo.z);
                macro_F = std::clamp<real>(macro_F,0.1,0.9);
                if(sel > macro_F)
                    lwi = sample_transmission(lwo,new_sam);
                else
                    lwi = sample_inner_reflect(lwo,new_sam);

                if(!lwi)
                    return {};

                const auto wi = shading_coord.local_to_global(lwi);
                const auto f = eval(wi,wo,mode);
                const auto pdf = this->pdf(wi,wo);
                return BSDFSampleResult{wi,f,pdf,false};
            }
            // sample for 4 case: specular, transmission, diffuse, clearcoat

            Vector3f lwi;

            real sel = sample.u;
            const Sample2 new_sam{sample.v,sample.w};

            if(sel < sample_weight.diffuse_weight)
                lwi = sample_diffuse(lwo,new_sam);
            else
                sel -= sample_weight.diffuse_weight;

            if(sel < sample_weight.transmission_weight)
                lwi = sample_transmission(lwo,new_sam);
            else
                sel -= sample_weight.transmission_weight;

            if(sel < sample_weight.specular_weight)
                lwi = sample_specular(lwo,new_sam);
            else
                lwi = sample_clearcoat(lwo,new_sam);

            if(!lwi)
                return {};
            const auto wi = shading_coord.local_to_global(lwi);
            const auto f = eval(wi,wo,mode);
            const auto pdf = this->pdf(wi,wo);
            return BSDFSampleResult{wi,f,pdf,false};
        }

        real pdf(const Vector3f& wi, const Vector3f& wo) const override{

            const auto lwi = shading_coord.global_to_local(wi).normalize();
            const auto lwo = shading_coord.global_to_local(wo).normalize();
            if(std::abs(lwi.z) < eps || std::abs(lwo.z) < eps)
                return 0;

            if(lwo.z < 0){
                if(!spec_trans)
                    return 0;

                real macro_F = dielectric_fresnel(ior,1,lwo.z);
                macro_F = std::clamp<real>(macro_F,0.1,0.9);
                if(lwi.z > 0)
                    return (1 - macro_F) * pdf_transmission(lwi,lwo);
                return macro_F * pdf_inner_reflect(lwi,lwo);
            }

            if(lwi.z < 0){
                return sample_weight.transmission_weight * pdf_transmission(lwi,lwo);
            }

            return sample_weight.diffuse_weight * pdf_diffuse(lwi,lwo) +
                   sample_weight.specular_weight * pdf_specular(lwi,lwo) +
                   sample_weight.clearcoat_weight * pdf_clearcoat(lwi,lwo);
        }

        bool is_delta() const override{
            return false;
        }

        bool has_diffuse() const override{
            return diffuse_weight > 0;
        }

        Spectrum get_albedo() const override{
            return C;
        }
    private:
        real pdf_transmission(const Vector3f& lwi,const Vector3f& lwo) const {
            const real eta = lwo.z > 0 ? ior : 1 / ior;
            const auto lwh = -(lwo + eta * lwi).normalize();

            if(((lwo.z > 0) != (dot(lwh, lwo) > 0)) ||
               ((lwi.z > 0) != (dot(lwh, lwi) > 0)))
                return 0;

            const real sdem = dot(lwo,lwh) + eta * dot(lwi,lwh);
            const real dwh_to_dwi = eta * eta * dot(lwi,lwh) / (sdem * sdem);

            const real phi_h = local_phi(lwh);
            const real sin_phi_h = std::sin(phi_h);
            const real cos_phi_h = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h,cos_phi_h,
                    sin_theta_h,cos_theta_h,ax,ay
                    );
            //todo replace cos_theta_h with dot(lwh,lwi) ? why?
            return std::abs(cos_theta_h * D * dwh_to_dwi);
        }
        real pdf_inner_reflect(const Vector3f& lwi,const Vector3f& lwo) const {
            const auto lwh = -(lwi + lwo).normalize();
            const real phi_h = local_phi(lwh);
            const real sin_phi_h = std::sin(phi_h);
            const real cos_phi_h = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real cos_theta_d = dot(lwi,lwh);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h,cos_phi_h,
                    sin_theta_h,cos_theta_h,ax,ay
            );
            return std::abs(cos_theta_h * D / (4 * cos_theta_d));
        }
        real pdf_diffuse(const Vector3f& lwi,const Vector3f& lwo) const {
            return CosineHemispherePdf(lwi.z);
        }
        //Sampling the GGX Distribution of Visible Normals
        //Appendix B
        real pdf_specular(const Vector3f& lwi,const Vector3f& lwo) const {
            const auto lwh = (lwi + lwo).normalize();
            const real phi_h = local_phi(lwh);
            const real sin_phi_h = std::sin(phi_h);
            const real cos_phi_h = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real cos_theta_d = dot(lwi,lwh);
            const real D = microfacet::anisotropic_gtr2(sin_phi_h,cos_phi_h,sin_theta_h,cos_theta_h,ax,ay);
            const real cos_phi_o = std::cos(local_phi(lwo));
            const real sin_phi_o = local_cos_to_sin(cos_phi_o);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(cos_phi_o,sin_phi_o,ax,ay,tan_theta_o);
            return std::abs(G * D / (4 * lwo.z));
        }
        real pdf_clearcoat(const Vector3f& lwi,const Vector3f& lwo) const {
            const auto lwh = (lwi + lwo).normalize();
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real cos_theta_d = dot(lwi,lwh);
            const real D = microfacet::gtr1(sin_theta_h,cos_theta_h,clearcoat_roughness);
            return std::abs(cos_theta_h * D / (4 * cos_theta_d));
        }
        Vector3f sample_transmission(const Vector3f& lwo,const Sample2& sample) const{
            const auto lwh = microfacet::sample_anisotropic_gtr2(ax,ay,sample);
            if(lwh.z <= 0)
                return {};

            // lwh lwo should in the same plane
            if((lwo.z > 0) != (dot(lwh,lwo) > 0))
                return {};

            const auto eta = lwo.z > 0 ? 1 / ior : ior;
            const auto ht = dot(lwh,lwo) > 0 ? lwh : -lwh;
            auto opt_lwi = refract_dir(lwo,ht,eta);
            if(!opt_lwi)
                return {};

            const auto lwi = opt_lwi->normalize();
            if(lwi.z * lwo.z > 0 || ((lwi.z > 0) != (dot(lwh, lwi) > 0)))
                return {};
            return lwi;
        }
        Vector3f sample_inner_reflect(const Vector3f& lwo,const Sample2& sample) const{
            const auto lwh = microfacet::sample_anisotropic_gtr2(ax,ay,sample).normalize();
            if(lwh.z <= 0)
                return {};
            const auto lwi = (2 * dot(lwo, lwh) * lwh - lwo);
            if(lwi.z > 0)
                return {};
            return lwi.normalize();
        }
        Vector3f sample_diffuse(const Vector3f& lwo,const Sample2& sample) const{
            return CosineSampleHemisphere(sample);
        }
        Vector3f sample_specular(const Vector3f& lwo,const Sample2& sample) const{
            const auto lwh = microfacet::sample_anisotropic_gtr2_with_visible_normal(lwo,ax,ay,sample).normalize();
            if(lwh.z <= 0)
                return {};
            const auto lwi = (2 * dot(lwo, lwh) * lwh - lwo);
            if(lwi.z > 0)
                return {};
            return lwi.normalize();
        }
        Vector3f sample_clearcoat(const Vector3f& lwo,const Sample2& sample) const{
            const auto lwh = microfacet::sample_gtr1(clearcoat_roughness,sample);
            if(lwh.z <= 0)
                return {};
            const auto lwi = (2 * dot(lwo, lwh) * lwh - lwo);
            if(lwi.z > 0)
                return {};
            return lwi.normalize();
        }

        Spectrum eval_transmission(const Vector3f& lwi,const Vector3f& lwo,TransportMode mode) const {
            const real cos_theta_i = lwi.z;
            const real cos_theta_o = lwo.z;
            const real eta = cos_theta_o > 0 ? ior : 1 / ior;
            const auto lwh = -(lwo + eta * lwi).normalize();
            const real cos_theta_d = dot(lwo,lwh);

            const real F = dielectric_fresnel(ior,1,cos_theta_d);

            const real phi_h = local_phi(lwh);
            const real sin_phi_h = std::sin(phi_h);
            const real cos_phi_h = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h,cos_phi_h,
                    sin_theta_h,cos_theta_h,
                    ax,ay
                    );

            const real phi_i = local_phi(lwi);
            const real phi_o = local_phi(lwo);
            const real sin_phi_i = std::sin(phi_i);
            const real cos_phi_i = std::cos(phi_i);
            const real sin_phi_o = std::sin(phi_o);
            const real cos_phi_o = std::cos(phi_o);
            const real tan_theta_i = local_tan_theta(lwi);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(cos_phi_i,sin_phi_i,
                                                              ax,ay,tan_theta_i) *
                          microfacet::smith_anisotropic_gtr2(cos_phi_o,sin_phi_o,
                                                             ax,ay,tan_theta_o);

            const real sdem = cos_theta_d + eta * dot(lwi,lwh);
            const real corr_factor = mode == TransportMode::Radiance ? 1 : eta * eta;
            const real value = (1 - F) * D * G * corr_factor * dot(lwi,lwh) * dot(lwo,lwh) /
                    (cos_theta_i * cos_theta_o * sdem * sdem);
            const real trans_factor = cos_theta_o > 0 ? spec_trans : 1;
            return (1 - metallic) * trans_factor * sqrt(C) * std::abs(value);
        }
        Spectrum eval_inner_reflect(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z < 0 && lwo.z < 0);
            const auto lwh = -(lwi + lwo).normalize();

            const real cos_theta_d = dot(lwo,lwh);
            const real F = dielectric_fresnel(ior,1,cos_theta_d);

            const real phi_h = local_phi(lwh);
            const real sin_phi_h = std::sin(phi_h);
            const real cos_phi_h = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h,cos_phi_h,
                    sin_theta_h,cos_theta_h,
                    ax,ay
            );

            const real phi_i = local_phi(lwi);
            const real phi_o = local_phi(lwo);
            const real sin_phi_i = std::sin(phi_i);
            const real cos_phi_i = std::cos(phi_i);
            const real sin_phi_o = std::sin(phi_o);
            const real cos_phi_o = std::cos(phi_o);
            const real tan_theta_i = local_tan_theta(lwi);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(cos_phi_i,sin_phi_i,
                                                              ax,ay,tan_theta_i) *
                           microfacet::smith_anisotropic_gtr2(cos_phi_o,sin_phi_o,
                                                              ax,ay,tan_theta_o);

            const auto value = F * D * G / (4 * lwi.z * lwo.z);
            return spec_trans * C * std::abs(value);
        }
        //diffuse + retro-reflect = disney 2012 diffuse
        Spectrum eval_diffuse(const Vector3f& lwi,const Vector3f& lwo) const{
            return C * invPI_r * (1 - real(0.5) * microfacet::one_minus_5(1 - lwi.z)) *
                    (1 - real(0.5) * microfacet::one_minus_5(1 - lwo.z));
        }
        Spectrum eval_retro_reflect(const Vector3f& lwi,const Vector3f& lwo) const{
            const real FL = microfacet::one_minus_5(lwi.z);
            const real FV = microfacet::one_minus_5(lwo.z);
            const auto lwh = (lwi + lwo).normalize();
            const auto cos_theta_d = dot(lwi,lwh);
            const real RR = 2 * roughness * square(cos_theta_d);
            return C * invPI_r * RR * (FL + FV + FL * FV * (RR - 1));
        }
        Spectrum eval_subsurface(const Vector3f& lwi,const Vector3f& lwo) const{
            const real FL = microfacet::one_minus_5(lwi.z);
            const real FV = microfacet::one_minus_5(lwo.z);
            const auto lwh = (lwi + lwo).normalize();
            const auto cos_theta_d = dot(lwi,lwh);
            real Fss90 = cos_theta_d * cos_theta_d * roughness;
            real Fss = mix<real>(1, Fss90, FL) * mix<real>(1, Fss90, FV);
            real ss = 1.25 * (Fss * (1.0 / (std::abs(lwi.z) + std::abs(lwo.z)) - 0.5) + 0.5);
            return C * invPI_r * ss;
        }
        Spectrum eval_sheen(const Vector3f& lwi,const Vector3f& lwo) const{
            const auto lwh = (lwi + lwo).normalize();
            const auto cos_theta_d = dot(lwi,lwh);
            return microfacet::one_minus_5(cos_theta_d) * sheen * mix(Spectrum(1),Ctint,sheen_tint);
        }
        static Spectrum schlick(const Spectrum &R0, real cos_theta) noexcept
        {
            return R0 + (Spectrum(1) - R0) * microfacet::one_minus_5(cos_theta);
        }
        Spectrum eval_specular(const Vector3f& lwi,const Vector3f& lwo) const{
            assert(lwi.z > 0 && lwo.z > 0);

            const auto lwh = -(lwi + lwo).normalize();

            const real cos_theta_d = dot(lwo,lwh);
            Spectrum Cspec0 = mix(mix(Spectrum(1), Ctint, specular_tint),
                                  C, metallic);
            const auto d_f = Cspec0 * dielectric_fresnel(ior,1,cos_theta_d);
            const auto c_f = schlick(Cspec0,cos_theta_d);
            const Spectrum F = mix(specular_scale * d_f, c_f, metallic);

            const real phi_h = local_phi(lwh);
            const real sin_phi_h = std::sin(phi_h);
            const real cos_phi_h = std::cos(phi_h);
            const real cos_theta_h = lwh.z;
            const real sin_theta_h = local_cos_to_sin(cos_theta_h);
            const real D = microfacet::anisotropic_gtr2(
                    sin_phi_h,cos_phi_h,
                    sin_theta_h,cos_theta_h,
                    ax,ay
            );

            const real phi_i = local_phi(lwi);
            const real phi_o = local_phi(lwo);
            const real sin_phi_i = std::sin(phi_i);
            const real cos_phi_i = std::cos(phi_i);
            const real sin_phi_o = std::sin(phi_o);
            const real cos_phi_o = std::cos(phi_o);
            const real tan_theta_i = local_tan_theta(lwi);
            const real tan_theta_o = local_tan_theta(lwo);
            const real G = microfacet::smith_anisotropic_gtr2(cos_phi_i,sin_phi_i,
                                                              ax,ay,tan_theta_i) *
                           microfacet::smith_anisotropic_gtr2(cos_phi_o,sin_phi_o,
                                                              ax,ay,tan_theta_o);

            return F * D * G / std::abs(4 * lwi.z * lwo.z);
        }
        Spectrum eval_clearcoat(const Vector3f& lwi,const Vector3f& lwo) const{
            const auto tan_theta_i = local_tan_theta(lwi);
            const auto tan_theta_o = local_tan_theta(lwo);
            const auto lwh = (lwi + lwo).normalize();
            const auto cos_theta_d = dot(lwh,lwi);
            const auto cos_theta_h = lwh.z;
            const auto sin_theta_h = local_cos_to_sin(cos_theta_h);
            const auto D = microfacet::gtr1(sin_theta_h,cos_theta_h,clearcoat_roughness);
            const auto F = schlick(Spectrum(0.04),cos_theta_d);
            const auto G = microfacet::smith_gtr2(tan_theta_i,real(0.25)) *
                    microfacet::smith_gtr2(tan_theta_o,real(0.25));
            return Spectrum(clearcoat * D * F * G / std::abs(4 * lwi.z * lwo.z));
        }
    };
}

//https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
//https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_slides.pdf

class Disney15: public Material{
public:
    Disney15(
            RC<const Texture2D> base_color,
            RC<const Texture2D> metallic,
            RC<const Texture2D> ior,
            RC<const Texture2D> roughness,
            RC<const Texture2D> specular,
            RC<const Texture2D> specular_tint,
            RC<const Texture2D> anisotropic,
            RC<const Texture2D> sheen,
            RC<const Texture2D> sheen_tint,
            RC<const Texture2D> clearcoat,
            RC<const Texture2D> clearcoat_gloss,
            RC<const Texture2D> spec_trans,
            RC<const Texture2D> scatter_dist,
            bool thin,
            RC<const Texture2D> flatness,
            RC<const Texture2D> diffuse_trans,
            Box<const NormalMapper> normal_mapper)
            :
            base_color(base_color),
            metallic(metallic),
            ior(ior),
            roughness(roughness),
            specular(specular),
            specular_tint(specular_tint),
            anisotropic(anisotropic),
            sheen(sheen),
            sheen_tint(sheen_tint),
            clearcoat(clearcoat),
            clearcoat_gloss(clearcoat_gloss),
            spec_trans(spec_trans),
            scatter_dist(scatter_dist),
            thin(thin),
            flatness(flatness),
            diffuse_trans(diffuse_trans),
            normal_mapper(std::move(normal_mapper)){

    }

    SurfaceShadingPoint shading(const SurfaceIntersection& isect, MemoryArena& arena) const override{
        auto uv = isect.uv;
        Spectrum base_color_ = base_color->evaluate(uv);
        real metallic_ = metallic->evaluate_s(uv);
        real ior_ = ior->evaluate_s(uv);
        real roughness_ = roughness->evaluate_s(uv);
        real specular_ = specular->evaluate_s(uv);
        real specular_tint_ = specular_tint->evaluate_s(uv);
        real anisotropic_ = anisotropic->evaluate_s(uv);
        real sheen_ = sheen->evaluate_s(uv);
        real sheen_tint_ = sheen_tint->evaluate_s(uv);
        real clearcoat_ = clearcoat->evaluate_s(uv);
        real clearcoat_gloss_ = clearcoat_gloss->evaluate_s(uv);
        real spec_trans_ = spec_trans->evaluate_s(uv);
        real scatter_dist_ = scatter_dist->evaluate_s(uv);
        real flatness_ = flatness->evaluate_s(uv);
        real diffuse_trans_ = diffuse_trans->evaluate_s(uv) / 2; // 0: all diffuse is reflected -> 1: transmitted
        const BSDF* bsdf = nullptr;
        const auto shading_coord = normal_mapper->reorient(uv,isect.shading_coord);
        if(thin){
            bsdf = arena.alloc_object<DisneyBSDF<true>>(
                    isect.geometry_coord,shading_coord,
                    base_color_,
                    metallic_,
                    ior_,
                    roughness_,
                    specular_,
                    specular_tint_,
                    anisotropic_,
                    sheen_,
                    sheen_tint_,
                    clearcoat_,
                    clearcoat_gloss_,
                    spec_trans_,
                    scatter_dist_,
                    flatness_,
                    diffuse_trans_
                    );
        }
        else{
            bsdf = arena.alloc_object<DisneyBSDF<false>>(
                    isect.geometry_coord,shading_coord,
                    base_color_,
                    metallic_,
                    ior_,
                    roughness_,
                    specular_,
                    specular_tint_,
                    anisotropic_,
                    sheen_,
                    sheen_tint_,
                    clearcoat_,
                    clearcoat_gloss_,
                    spec_trans_,
                    scatter_dist_,
                    flatness_,
                    diffuse_trans_
                    );
        }
        const BSSRDF* bssrdf = nullptr;
        real diffuse_weight = (1 - metallic_) * (1 - spec_trans_);
        if(diffuse_weight > 0 && !thin && scatter_dist_ > 0){
            bssrdf = arena.alloc_object<DisneyBSSRDF>(isect,ior_,base_color_,Spectrum(scatter_dist_));
        }
        SurfaceShadingPoint shd_p;
        shd_p.bssrdf = bssrdf;
        shd_p.shading_n = isect.shading_coord.z;
        shd_p.bsdf = bsdf;
        return shd_p;
    }
private:
    RC<const Texture2D> base_color;
    RC<const Texture2D> metallic;
    RC<const Texture2D> ior;
    RC<const Texture2D> roughness;
    RC<const Texture2D> specular;
    RC<const Texture2D> specular_tint;
    RC<const Texture2D> anisotropic;
    RC<const Texture2D> sheen;
    RC<const Texture2D> sheen_tint;
    RC<const Texture2D> clearcoat;
    RC<const Texture2D> clearcoat_gloss;
    RC<const Texture2D> spec_trans;
    RC<const Texture2D> scatter_dist;
    bool thin;
    RC<const Texture2D> flatness;
    RC<const Texture2D> diffuse_trans;
    Box<const NormalMapper> normal_mapper;
};




RC<Material> create_disney_bsdf(
        RC<const Texture2D> base_color,
        RC<const Texture2D> metallic,
        RC<const Texture2D> ior,
        RC<const Texture2D> roughness,
        RC<const Texture2D> specular,
        RC<const Texture2D> specular_tint,
        RC<const Texture2D> anisotropic,
        RC<const Texture2D> sheen,
        RC<const Texture2D> sheen_tint,
        RC<const Texture2D> clearcoat,
        RC<const Texture2D> clearcoat_gloss,
        RC<const Texture2D> spec_trans,
        RC<const Texture2D> scatter_dist,
        bool thin,
        RC<const Texture2D> flatness,
        RC<const Texture2D> diffuse_trans,
        Box<const NormalMapper> normal_mapper
){
    return newRC<Disney15>(
            base_color,
            metallic,
            ior,
            roughness,
            specular,
            specular_tint,
            anisotropic,
            sheen,
            sheen_tint,
            clearcoat,
            clearcoat_gloss,
            spec_trans,
            scatter_dist,
            thin,
            flatness,
            diffuse_trans,
            std::move(normal_mapper)
            );
}

TRACER_END