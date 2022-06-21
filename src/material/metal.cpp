//
// Created by wyz on 2022/6/20.
//
#include "core/material.hpp"

#include "utility/fresnel_point.hpp"
#include "aggregate_bsdf.hpp"
#include "bxdf/microfacet_refl_bxdf.hpp"

TRACER_BEGIN

class Metal: public Material{
public:
    Metal(RC<const Texture2D> color,
          RC<const Texture2D> k,
          RC<const Texture2D> eta,
          RC<const Texture2D> roughness,
          RC<const Texture2D> anisotropic,
          Box<NormalMapper> normal_mapper)
          :color_(color),
          k_(k),
          eta_(eta),
          roughness_(roughness),
          anisotropic_(anisotropic),
          normal_mapper_(std::move(normal_mapper))
          {}

    virtual Spectrum evaluate(const Point2f& uv) const  override{
        return {};
    }

    virtual SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const override{
        const Coord shading_coord = normal_mapper_->reorient(
                isect.uv, isect.shading_coord);

        const Spectrum color   = color_->evaluate(isect.uv);
        const Spectrum k       = k_->evaluate(isect.uv);
        const Spectrum eta     = eta_->evaluate(isect.uv);
        const real roughness   = roughness_->evaluate_s(isect.uv);
        const real anisotropic = anisotropic_->evaluate_s(isect.uv);

        const auto fresnel = arena.alloc_object<PaintedConductorFresnelPoint>(
                color, Spectrum(1), eta, k);

        auto *bsdf = arena.alloc_object<AggregateBSDF<1>>(
                isect.geometry_coord, shading_coord, color);
        bsdf->add_bxdf(arena.alloc_object<GGXMicrofacetReflectionBXDF>(
                fresnel, roughness, anisotropic),1);

        SurfaceShadingPoint shd;
        shd.bsdf = bsdf;
        shd.shading_n = shading_coord.z;
        return shd;
    }

private:
    RC<const Texture2D> color_;

    RC<const Texture2D> k_;
    RC<const Texture2D> eta_;

    RC<const Texture2D> roughness_;
    RC<const Texture2D> anisotropic_;

    Box<NormalMapper> normal_mapper_;
};

RC<Material> create_metal(
        RC<const Texture2D> color,
        RC<const Texture2D> eta,
        RC<const Texture2D> k,
        RC<const Texture2D> roughness,
        RC<const Texture2D> anisotropic,
        Box<NormalMapper> normal_mapper){
    return newRC<Metal>(color,eta,k,roughness,anisotropic,std::move(normal_mapper));
}


TRACER_END