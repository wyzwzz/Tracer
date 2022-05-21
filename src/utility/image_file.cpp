//
// Created by wyz on 2022/5/18.
//
#include "image_file.hpp"
#include "utility/mesh_load.hpp"
#include "utility/logger.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "factory/texture.hpp"
TRACER_BEGIN

    void write_image_to_hdr(const Image2D<Spectrum>& image,
                            const std::string& filename){
        stbi_write_hdr(filename.c_str(),
                       image.width(),
                       image.height(),
                       3,
                       reinterpret_cast<const float*>(image.get_raw_data()));
    }

    RC<Image2D<Color3b>> load_image_from_file(const std::string& filename){
        int w,h,nComp;
        auto d = stbi_load(filename.c_str(),&w,&h,&nComp,0);
        if(!d){
            throw std::runtime_error("load image failed");
        }
        if(nComp == 3){
            return newRC<Image2D<Color3b>>(w,h,reinterpret_cast<Color3b*>(d));
        }
        else if(nComp == 4){
            LOG_INFO("load image with 4 component");
            auto image = newRC<Image2D<Color3b>>(w,h);
            auto p = image->get_raw_data();
            for(int i = 0; i < w * h; ++i){
                p[i] = {d[i*3],d[i*3+1],d[i*3+2]};
            }
            return image;
        }
        else{
            throw std::runtime_error("invalid image component");
        }
    }

    RC<Image2D<Color3f>> load_hdr_from_file(const std::string& filename){
        int w,h,nComp;
        auto d = stbi_loadf(filename.c_str(),&w,&h,&nComp,0);
        if(!d){
            throw std::runtime_error("load image failed");
        }
        if(nComp == 3){
            return newRC<Image2D<Color3f>>(w,h,reinterpret_cast<Color3f*>(d));
        }
        else if(nComp == 4){
            LOG_INFO("load image with 4 component");
            auto image = newRC<Image2D<Color3f>>(w,h);
            auto p = image->get_raw_data();
            for(int i = 0; i < w * h; ++i){
                p[i] = {d[i*3],d[i*3+1],d[i*3+2]};
            }
            return image;
        }
        else{
            throw std::runtime_error("invalid image component");
        }
    }

    bool is_float_image(const std::string& filename){
        assert(!filename.empty());
        auto ext = filename.substr(filename.find_last_of("."));
        if(ext.empty()){
            throw std::runtime_error("invalid filename for image");
        }
        if(ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == "bmp"){
            return false;
        }
        else if(ext == ".hdr"){
            return true;
        }
        else{
            throw std::runtime_error("unsupported image format");
        }
    }

    static RC<Texture2D> _create_texture_from_file(const std::string& name,real scale_r,real scale_g,real scale_b){
        Spectrum constant = {scale_r,scale_g,scale_b};
        if(name.empty()){
            LOG_INFO("create constant texture: {} {} {}",scale_r,scale_g,scale_b);
            return create_constant_texture2d(constant);
        }
        else{
            LOG_INFO("load texture from file: {}, scale: {} {} {}",name,scale_r,scale_g,scale_b);
            if(!is_float_image(name)){
                auto image = load_image_from_file(name);
                image->operator*=( Color3f(scale_r,scale_g,scale_b));
                return create_image_texture2d(image);
            }
            else{
                auto image = load_hdr_from_file(name);
                image->operator*=( Color3f(scale_r,scale_g,scale_b));
                return create_hdr_texture2d(image);
            }
        }
    }
    static RC<Texture2D> _create_texture_from_file(const std::string& name,const real* scale){
        assert(scale);
        return _create_texture_from_file(name,scale[0],scale[1],scale[2]);
    }
    static RC<Texture2D> _create_texture_from_file(const std::string& name,real scale){
        return _create_texture_from_file(name,scale,scale,scale);
    }

    MaterialTexture create_texture_from_file(const material_t& material){
        MaterialTexture textures;
        real one[3] = {1.0,1.0,1.0};

        textures.map_ka = _create_texture_from_file(material.map_ka,material.ambient);
        textures.map_kd = _create_texture_from_file(material.map_kd,material.diffuse);
        textures.map_ks = _create_texture_from_file(material.map_ks,material.specular);
        textures.map_ns = _create_texture_from_file(material.map_ns,material.shininess);
        //todo light



        return textures;
    }


TRACER_END