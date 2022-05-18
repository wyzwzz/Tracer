//
// Created by wyz on 2022/5/18.
//
#include "image_file.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
TRACER_BEGIN

    void write_image_to_hdr(const Image2D<Spectrum>& image,
                            const std::string filename){
        stbi_write_hdr(filename.c_str(),
                       image.width(),
                       image.height(),
                       3,
                       reinterpret_cast<const float*>(image.get_raw_data()));
    }

TRACER_END