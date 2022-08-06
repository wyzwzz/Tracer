//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_FACTORY_MATERIAL_HPP
#define TRACER_FACTORY_MATERIAL_HPP


#include "core/material.hpp"
#include "core/texture.hpp"
TRACER_BEGIN

RC<Material> create_glass(
        RC<const Texture2D> map_kr,
        RC<const Texture2D> map_kt,
        RC<const Texture2D> map_ior
        );

RC<Material> create_phong_material(
        RC<const Texture2D> map_ka,
        RC<const Texture2D> map_kd,
        RC<const Texture2D> map_ks,
        RC<const Texture2D> map_ns
        );

RC<Material> create_invisible_surface(RC<const BSSRDFSurface> bssrdf_surface);

RC<Material> create_invisible_surface();


RC<BSSRDFSurface> create_normalized_diffusion_bssrdf_surface(
        RC<const Texture2D> A,
        RC<const Texture2D> dmfp,
        RC<const Texture2D> eta);

//2015 bsdf version
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
        RC<const BSSRDFSurface> bssrdf);

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
        );


RC<Material> create_metal(
        RC<const Texture2D> color,
        RC<const Texture2D> eta,
        RC<const Texture2D> k,
        RC<const Texture2D> roughness,
        RC<const Texture2D> anisotropic,
        Box<NormalMapper> normal_mapper);


TRACER_END

#endif //TRACER_FACTORY_MATERIAL_HPP
