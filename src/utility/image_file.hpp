//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_IMAGE_FILE_HPP
#define TRACER_IMAGE_FILE_HPP
#include "core/render.hpp"
#include <string>
TRACER_BEGIN

void write_image_to_hdr(const Image2D<Spectrum>&,
                        const std::string filename);


TRACER_END

#endif //TRACER_IMAGE_FILE_HPP
