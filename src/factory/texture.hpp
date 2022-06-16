//
// Created by wyz on 2022/5/21.
//

#ifndef TRACER_FACTORY_TEXTURE_HPP
#define TRACER_FACTORY_TEXTURE_HPP

#include "core/texture.hpp"
#include "utility/image.hpp"
TRACER_BEGIN

RC<Texture2D> create_constant_texture2d(const Spectrum& constant);

RC<Texture2D> create_image_texture2d(const RC<Image2D<Color3b>>& image);

RC<Texture2D> create_hdr_texture2d(const RC<Image2D<Color3f>>& image);

RC<Texture3D> create_constant_texture3d(const Spectrum& constant);

RC<Texture3D> create_image_texture3d(const RC<Image3D<real>>& image);

RC<Texture3D> create_image_texture3d(const RC<Image3D<uint8_t>>& image);

RC<Texture3D> create_image_texture3d(const RC<Image3D<Color3f>>& image);

RC<Texture3D> create_image_texture3d(const RC<Image3D<Color3b>>& image);

TRACER_END

#endif //TRACER_FACTORY_TEXTURE_HPP
