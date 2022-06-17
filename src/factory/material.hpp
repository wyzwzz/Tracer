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


TRACER_END

#endif //TRACER_FACTORY_MATERIAL_HPP
