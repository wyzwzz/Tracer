//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_IMAGE_FILE_HPP
#define TRACER_IMAGE_FILE_HPP
#include "core/render.hpp"
#include <string>
#include "core/texture.hpp"
TRACER_BEGIN

void write_image_to_hdr(const Image2D<Spectrum>&,
                        const std::string& filename);

    void write_image_to_png(const Image2D<Color3b>& image,const std::string& filename);

RC<Image2D<Color3b>> load_image_from_file(const std::string& filename);

RC<Image2D<Color3f>> load_hdr_from_file(const std::string& filename);

struct MaterialTexture{
    RC<Texture2D> map_ka;
    RC<Texture2D> map_kd;
    RC<Texture2D> map_ks;
    RC<Texture2D> map_ns;
    RC<Texture2D> map_bump;
    RC<Texture2D> disp;
    RC<Texture2D> map_d;//displayment
    RC<Texture2D> refl;

    //PBR extension
    RC<Texture2D> map_pr;//roughness
    RC<Texture2D> map_pm;//metallic
    RC<Texture2D> map_ps;//sheen
    RC<Texture2D> map_ke;
    RC<Texture2D> norm;
    bool has_emission = false;
};
struct material_t;
MaterialTexture create_texture_from_file(const material_t& material);

TRACER_END

#endif //TRACER_IMAGE_FILE_HPP
