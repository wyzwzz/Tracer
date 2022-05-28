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
    void write_image_to_png(const Image2D<Color3b>& image,const std::string& filename){
        stbi_write_png(filename.c_str(),image.width(),image.height(),3,image.get_raw_data(),0);
    }

    RC<Image2D<Color3b>> load_image_from_file(const std::string& filename){
        stbi_set_flip_vertically_on_load(true);
        int w,h,nComp;
        auto d = stbi_load(filename.c_str(),&w,&h,&nComp,0);
        if(!d){
            throw std::runtime_error("load image failed");
        }
        if(nComp == 3){
            return newRC<Image2D<Color3b>>(w,h,reinterpret_cast<Color3b*>(d));
        }
        else if(nComp == 1){
            LOG_INFO("load image with 1 component");
            auto image = newRC<Image2D<Color3b>>(w,h);
            auto p = image->get_raw_data();
            for(int i = 0; i < w * h; ++i){
                p[i] = {d[i],d[i],d[i]};
            }
            return image;
        }
        else if(nComp == 4){
            LOG_INFO("load image with 4 component");
            auto image = newRC<Image2D<Color3b>>(w,h);
            auto p = image->get_raw_data();
            for(int i = 0; i < w * h; ++i){
                p[i] = {d[i*4],d[i*4+1],d[i*4+2]};
            }
            return image;
        }
        else{
            throw std::runtime_error("invalid image component");
        }
    }

    RC<Image2D<Color3f>> load_hdr_from_file(const std::string& filename){
        stbi_set_flip_vertically_on_load(false);
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
            if(!scale_r && !scale_g && !scale_b){
                scale_r = scale_g = scale_b = 1;
            }
            LOG_INFO("load texture from file: {}, scale: {} {} {}",name,scale_r,scale_g,scale_b);
            if(!is_float_image(name)){
                auto image = load_image_from_file(name);
                image->map([](const Color3b& c){
                   Color3f t{(real)c.x/255,(real)c.y/255,(real)c.z/255};
                   real gamma =  2.2;
                   t.x = std::pow(t.x,gamma);
                   t.y = std::pow(t.y,gamma);
                   t.z = std::pow(t.z,gamma);
                   return Color3b(t.x*255,t.y*255,t.z*255);
                });
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
        LOG_INFO("create specular texture: {} {} {}",material.specular[0],material.specular[1],material.specular[2]);
        textures.map_ns = _create_texture_from_file(material.map_ns,material.shininess);
        //todo light
        textures.map_ke = _create_texture_from_file(material.map_ke,material.emission);
        if(material.emission[0] > 0 || material.emission[1] > 0 || material.emission[2] > 0){
            textures.has_emission = true;
        }
        if(material.ior > 1 ){
            LOG_INFO("find transparent material with ior: {}",material.ior);
            if(textures.map_ks->evaluate_s({0.5,0.5}) == 0){
                textures.map_ks = _create_texture_from_file("",one);
            }
            textures.map_kt = textures.map_ks;
            textures.as_glass = true;
            textures.ior = _create_texture_from_file("",material.ior);
        }



        return textures;
    }

    RC<const Texture2D> create_texture2d_from_file(const std::string& filename){

        if(!is_float_image(filename)){
            auto img = load_image_from_file(filename);
            return create_image_texture2d(img);
        }
        else{
            auto img = load_hdr_from_file(filename);
            return create_hdr_texture2d(img);
        }
    }

TRACER_END