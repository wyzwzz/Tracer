//
// Created by wyz on 2022/6/16.
//
#include "../core/texture.hpp"
#include "../utility/image.hpp"
#include "../core/spectrum.hpp"
#include "utility/sampler.hpp"
#include <cassert>
TRACER_BEGIN

template<typename T>
class ImageTexture3D: public Texture3D{
public:

    ImageTexture3D(const RC<const Image3D<T>>& data_){
        this->data = std::move(data_);
        max_spectrum_ = Spectrum(REAL_MIN);
        max_real_ = REAL_MIN;
        for(int z = 0; z < data->depth(); ++z){
            for(int y = 0; y < data->height(); ++y){
                for(int x = 0; x < data->width(); ++x){
                    if constexpr(std::is_same_v<T,real>){
                        max_real_ = std::max(max_real_,data->at(x,y,z));
                    }
                    else if constexpr(std::is_same_v<T,uint8_t>){
                        max_real_ = std::max(max_real_,data->at(x,y,z) / real(255));
                    }
                    else if constexpr(std::is_same_v<T,Color3b>){
                        auto t = data->at(x,y,z);
                        Spectrum spec(t[0] / real(255),
                                      t[1] / real(255),
                                      t[2] / real(255));
                        max_spectrum_.r = std::max(max_spectrum_.r,spec.r);
                        max_spectrum_.g = std::max(max_spectrum_.g,spec.g);
                        max_spectrum_.b = std::max(max_spectrum_.b,spec.b);
                    }
                    else{
                        bool ok = std::is_same_v<T,Color3f>;
                        assert(ok);
                        auto t = data->at(x,y,z);
                        max_spectrum_.r = std::max(max_spectrum_.r,t[0]);
                        max_spectrum_.g = std::max(max_spectrum_.g,t[1]);
                        max_spectrum_.b = std::max(max_spectrum_.b,t[2]);
                    }
                }
            }
        }
        if constexpr(std::is_same_v<T,real> || std::is_same_v<T,uint8_t>){
            max_spectrum_ = Spectrum(max_real_);
        }
        else{
            max_real_ = max_spectrum_.r;
        }
    }

    ~ImageTexture3D() = default;

    int width() const noexcept override{
        return data->width();
    }

    int height() const noexcept override{
        return data->height();
    }

    int depth() const noexcept override{
        return data->depth();
    }

    Spectrum evaluate_impl(const Point3f& uvw) const noexcept override{
        auto v = LinearSampler::Sample3D(*data.get(),uvw.x,uvw.y,uvw.z);
        if constexpr(std::is_same_v<T,real>){
            return Spectrum(v);
        }
        else if constexpr(std::is_same_v<T,uint8_t>){
            return Spectrum(v/real(255.0));
        }
        else if constexpr(std::is_same_v<T,Color3b>){
            return Spectrum(v[0]/real(255.0),v[1]/real(255.0),v[2]/real(255.0));
        }
        else{
            return Spectrum(v[0],v[1],v[2]);
        }
    }

    Spectrum max_spectrum() const noexcept override{
        return max_spectrum_;
    }

    real max_real() const noexcept override{
        return max_real_;
    }
private:
    RC<const Image3D<T>> data;
    Spectrum max_spectrum_;
    real max_real_;
};



RC<Texture3D> create_image_texture3d(const RC<Image3D<real>>& image){
    return newRC<ImageTexture3D<real>>(image);
}

RC<Texture3D> create_image_texture3d(const RC<Image3D<uint8_t>>& image){
    return newRC<ImageTexture3D<uint8_t>>(image);
}

RC<Texture3D> create_image_texture3d(const RC<Image3D<Color3f>>& image){
    return newRC<ImageTexture3D<Color3f>>(image);
}

RC<Texture3D> create_image_texture3d(const RC<Image3D<Color3b>>& image){
    return newRC<ImageTexture3D<Color3b>>(image);
}

TRACER_END