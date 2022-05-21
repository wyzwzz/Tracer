//
// Created by wyz on 2022/5/21.
//

#ifndef TRACER_FACTORY_TEXTURE_HPP
#define TRACER_FACTORY_TEXTURE_HPP

#include "core/texture.hpp"

TRACER_BEGIN

RC<Texture2D> create_constant_texture2d(const Spectrum& constant);

RC<Texture2D> create_image_texture2d(const RC<Image2D<Color3b>>& image);

RC<Texture2D> create_hdr_texture2d(const RC<Image2D<Color3f>>& image);

TRACER_END

#endif //TRACER_FACTORY_TEXTURE_HPP
