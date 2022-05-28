//
// Created by wyz on 2022/5/21.
//
#include "core/texture.hpp"
#include "utility/image.hpp"
#include "utility/sampler.hpp"
TRACER_BEGIN

    class HDRTexture2D: public Texture2D{
    public:
        HDRTexture2D(const RC<Image2D<Color3f>>& image,real gamma = 1)
        :data(image)
        {
            this->inv_gamma = 1.0 / gamma;
        }

        ~HDRTexture2D() override {}

        int width() const noexcept {
            return data->width();
        };

        int height() const noexcept {
            return data->height();
        }

        Spectrum evaluate_impl(const Point2f& uv) const noexcept override{
            auto v = LinearSampler::Sample2D(*data.get(),uv.x,uv.y);
            return Spectrum(v.x,v.y,v.z);
        }

    private:
        RC<Image2D<Color3f>> data;

    };

    RC<Texture2D> create_hdr_texture2d(const RC<Image2D<Color3f>>& image){
        return newRC<HDRTexture2D>(image);
    }

TRACER_END