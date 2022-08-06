#include "core/bsdf.hpp"
#include "core/material.hpp"
#include "core/texture.hpp"
#include "utility/reflection.hpp"
#include "utility/microfacet.hpp"


TRACER_BEGIN

namespace {


    class DisneyBRDF: public LocalBSDF{
        Spectrum base_color;
        real subsurface;
        real metallic;
        real specular;
        real specular_tint;
        real roughness;
        real anisotropic;
        real sheen;
        real sheen_tint;
        real clearcoat;
        real clearcoat_gloss;
    public:
        DisneyBRDF(const Coord &geometry_coord, const Coord &shading_coord,
                   const Spectrum& base_color,
                   real subsurface,
                   real metallic,
                   real specular,
                   real specular_tint,
                   real roughness,
                   real anisotropic,
                   real sheen,
                   real sheen_tint,
                   real clearcoat,
                   real clearcoat_gloss
                   )
        : LocalBSDF(geometry_coord,shading_coord),
        base_color(base_color),
        subsurface(subsurface),
        metallic(metallic),
        specular(specular),
        specular_tint(specular_tint),
        roughness(roughness),
        anisotropic(anisotropic),
        sheen(sheen),
        sheen_tint(sheen_tint),
        clearcoat(clearcoat),
        clearcoat_gloss(clearcoat_gloss)
        {

        }


        Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
            if(cause_black_fringes(wi,wo))
                return evaluate_black_fringes(wi,wo);

            //global to local
            const auto lwi = shading_coord.global_to_local(wi).normalize();
            const auto lwo = shading_coord.global_to_local(wo).normalize();
            if(lwi.z < eps || lwo.z < eps)
                return {};
            const real NdotL = lwi.z;
            const real NdotV = lwo.z;
            const auto H = normalize(lwi + lwo);
            const real NdotH = H.z;
            const real LdotH = dot(lwi,H);

            real lum = base_color.lum();

            Spectrum Ctint = lum > 0 ? base_color / lum : Spectrum(1);
            Spectrum Cspec0 = mix(specular * 0.08f * mix(Spectrum(1), Ctint, specular_tint),
                                  base_color, metallic);
            Spectrum Csheen = mix(Spectrum(1), Ctint, sheen_tint);

            Spectrum f_disney;

            //specular
            real aspect = std::sqrt(1 - anisotropic * 0.9);
            real ax = std::max<real>(0.001,roughness * roughness / aspect);
            real ay = std::max<real>(0.001,roughness * roughness * aspect);
            real phi_h = local_phi(H);
            real sin_phi_h = std::sin(phi_h);
            real cos_phi_h = std::cos(phi_h);
            real Ds = microfacet::anisotropic_gtr2(sin_phi_h,
                                                   cos_phi_h,
                                                   local_cos_to_sin(H.z),
                                                   H.z,
                                                   ax,ay);
            real FH = microfacet::one_minus_5(LdotH);
            Spectrum Fs = mix(Cspec0,Spectrum(1),FH);
            real phi_i = local_phi(lwi);
            real sin_phi_i = std::sin(phi_i);
            real cos_phi_i = std::cos(phi_i);
            real Gs = microfacet::smith_anisotropic_gtr2(sin_phi_i,
                                                         cos_phi_i,
                                                         ax,ay,
                                                         local_tan_theta(lwi));
            real phi_o = local_phi(lwo);
            real sin_phi_o = std::sin(phi_o);
            real cos_phi_o = std::cos(phi_o);
            Gs *= microfacet::smith_anisotropic_gtr2(sin_phi_o,
                                                     cos_phi_o,
                                                     ax,ay,
                                                     local_tan_theta(lwo));
            //clearcoat (ior = 1.5 -> F0 = 0.04)
            real Dr = microfacet::gtr1(local_cos_to_sin(H.z),H.z,mix<real>(0.1,0.001,clearcoat_gloss));
            real Fr = mix<real>(0.04,1.0,FH);
            real Gr = microfacet::smith_gtr2(local_tan_theta(lwi),0.25)
                    * microfacet::smith_gtr2(local_tan_theta(lwo),0.25);

            f_disney += (Ds * Fs * Gs + 0.25f * clearcoat * Gr * Fr * Dr) / (4 * NdotL * NdotV);
            if(metallic < 1){
                //has diffuse and surface and sheen
                //diffuse
                real FL = microfacet::one_minus_5(NdotL);
                real FV = microfacet::one_minus_5(NdotV);
                real Fd90 = 0.5 + 2. * LdotH * LdotH * roughness;
                real Fd = mix<real>(1, Fd90, FL) * mix<real>(1, Fd90, FV);
                //subsurface
                real Fss90 = LdotH * LdotH * roughness;
                real Fss = mix<real>(1, Fss90, FL) * mix<real>(1, Fss90, FV);
                real ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);
                //sheen
                Spectrum Fsheen = FH * sheen * Csheen;

                f_disney += (1 - metallic) * (base_color * invPI_r  *
                        mix(Fd, ss, subsurface) + Fsheen);
            }
            if(!f_disney.is_valid()){
                LOG_CRITICAL("invalid f");
            }
            return f_disney;
        }

        BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sample) const override{
            if(cause_black_fringes(wo))
                return sample_black_fringes(wo, mode, sample);

            real p_diffuse = std::min<real>(0.8,1 - metallic);
            real p_specular = 1.0 / (1.0 + 0.5 * clearcoat) * (1 - p_diffuse);

            const auto lwo = shading_coord.global_to_local(wo).normalize();
            Vector3f lwi;
            real sel = sample.u;
            const Sample2 new_sample = {sample.v,sample.w};
            real aspect = std::sqrt(1 - anisotropic * 0.9);
            real ax = std::max<real>(0.001,roughness * roughness / aspect);
            real ay = std::max<real>(0.001,roughness * roughness * aspect);
            if(sel < p_diffuse){
                lwi = CosineSampleHemisphere(new_sample);
            }
            sel -= p_diffuse;
            if(sel < p_specular){
                const auto lwh = microfacet::sample_anisotropic_gtr2(ax,ay,new_sample);
                if(lwh.z > 0){
                    lwi = (2 * dot(lwo,lwh) * lwh - lwo).normalize();
                    if(lwi.z <= 0)
                        lwi = {};
                }
            }
            else{
                const auto lwh = microfacet::sample_gtr1(clearcoat_gloss,new_sample);
                if(lwh.z > 0){
                    lwi = (2 * dot(lwo,lwh) * lwh - lwo).normalize();
                    if(lwi.z <= 0)
                        lwi = {};
                }
            }
            if(!lwi) return {};
            BSDFSampleResult ret;
            ret.wi = shading_coord.local_to_global(lwi);
            if(!isfinite(ret.pdf) || std::isnan(ret.pdf)){
                LOG_ERROR("pdf get nan");
            }
            ret.f = eval(ret.wi,wo,mode);
            ret.pdf = pdf(ret.wi,wo);
            ret.is_delta = false;
            return ret;
        }

        real pdf(const Vector3f& wi, const Vector3f& wo) const override{
            if(cause_black_fringes(wi, wo))
                return pdf_black_fringes(wi, wo);

            real p_diffuse = std::min<real>(0.8,1 - metallic);
            real p_specular = 1.0 / (1.0 + 0.5 * clearcoat);

            real aspect = std::sqrt(1 - anisotropic * 0.9);
            real ax = std::max<real>(0.001,roughness * roughness / aspect);
            real ay = std::max<real>(0.001,roughness * roughness * aspect);
            const Vector3f lwi = shading_coord.global_to_local(wi).normalize();
            const Vector3f lwo = shading_coord.global_to_local(wo).normalize();

            if(lwi.z < eps) return 0;

            const auto H = normalize(lwi + lwo);

            real pdf_diffuse = CosineHemispherePdf(lwi.z);
            real phi_h = local_phi(H);
            real sin_phi_h = std::sin(phi_h);
            real cos_phi_h = std::cos(phi_h);
            real pdf_specular =
                    microfacet::anisotropic_gtr2(sin_phi_h,
                                                 cos_phi_h,
                                                       local_cos_to_sin(H.z),H.z,
                                                       ax,ay) * H.z
                                                       / (4 * dot(lwi,H));
            real pdf_clearcoat =
                    microfacet::gtr1(sin_phi_h,
                                     cos_phi_h,
                                     clearcoat_gloss) * H.z / (4 * dot(lwi,H));



            real pdf =  p_diffuse * pdf_diffuse + (1 - pdf_diffuse) * (
                    p_specular * pdf_specular + (1 - p_specular) * pdf_clearcoat
                    );
            if(!isfinite(pdf)){
                LOG_CRITICAL("invalid pdf: {} {} {} {}",pdf,pdf_diffuse,pdf_specular,pdf_clearcoat);
                LOG_CRITICAL("invalid H: {} {} {}",H.x,H.y,H.z);
                return 0;
            }
            return pdf;
        }

        bool is_delta() const override{
            return false;
        }


        bool has_diffuse() const override{
            return metallic < 1;
        }

        Spectrum get_albedo() const override{
            return base_color;
        }


    };

}

class Disney12: public Material{
public:
    Disney12(
            RC<const Texture2D> base_color,
            RC<const Texture2D> subsurface,
            RC<const Texture2D> metallic,
            RC<const Texture2D> specular,
            RC<const Texture2D> specular_tint,
            RC<const Texture2D> roughness,
            RC<const Texture2D> anisotropic,
            RC<const Texture2D> sheen,
            RC<const Texture2D> sheen_tint,
            RC<const Texture2D> clearcoat,
            RC<const Texture2D> clearcoat_gloss
            )
    :
    base_color(base_color),
    subsurface(subsurface),
    metallic(metallic),
    specular(specular),
    specular_tint(specular_tint),
    roughness(roughness),
    anisotropic(anisotropic),
    sheen(sheen),
    sheen_tint(sheen_tint),
    clearcoat(clearcoat),
    clearcoat_gloss(clearcoat_gloss)
    {

    }

    SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const override{
        const Point2f uv = isect.uv;
        const Spectrum base_color_ = base_color->evaluate(uv);
        const real subsurface_ = subsurface->evaluate_s(uv);
        const real metallic_ = metallic->evaluate_s(uv);
        const real specular_ = specular->evaluate_s(uv);
        const real specular_tint_ = specular_tint->evaluate_s(uv);
        const real roughness_ = roughness->evaluate_s(uv);
        const real anisotropic_ = anisotropic->evaluate_s(uv);
        const real sheen_ = sheen->evaluate_s(uv);
        const real sheen_tint_ = sheen_tint->evaluate_s(uv);
        const real clearcoat_ = clearcoat->evaluate_s(uv);
        const real clearcoat_gloss_ = clearcoat_gloss->evaluate_s(uv);

        const BSDF* bsdf = arena.alloc_object<DisneyBRDF>(
                isect.geometry_coord,isect.shading_coord,
                base_color_,
                subsurface_,metallic_,
                specular_,specular_tint_,
                roughness_,anisotropic_,
                sheen_,sheen_tint_,
                clearcoat_,clearcoat_gloss_
                );
        return SurfaceShadingPoint{bsdf,isect.shading_coord.z,nullptr};
    }

private:
    RC<const Texture2D> base_color;
    RC<const Texture2D> subsurface;
    RC<const Texture2D> metallic;
    RC<const Texture2D> specular;
    RC<const Texture2D> specular_tint;
    RC<const Texture2D> roughness;
    RC<const Texture2D> anisotropic;
    RC<const Texture2D> sheen;
    RC<const Texture2D> sheen_tint;
    RC<const Texture2D> clearcoat;
    RC<const Texture2D> clearcoat_gloss;
};


//2012 brdf version
RC<Material> create_disney_brdf(
        RC<const Texture2D> base_color,
        RC<const Texture2D> subsurface,
        RC<const Texture2D> metallic,
        RC<const Texture2D> specular,
        RC<const Texture2D> specular_tint,
        RC<const Texture2D> roughness,
        RC<const Texture2D> anisotropic,
        RC<const Texture2D> sheen,
        RC<const Texture2D> sheen_tint,
        RC<const Texture2D> clearcoat,
        RC<const Texture2D> clearcoat_gloss
){
    return newRC<Disney12>(base_color,
                           subsurface,
                           metallic,
                           specular,
                           specular_tint,
                           roughness,
                           anisotropic,
                           sheen,
                           sheen_tint,
                           clearcoat,
                           clearcoat_gloss);
}

TRACER_END