//
// Created by wyz on 2022/5/20.
//
#include "core/texture.hpp"
#include "utility/image.hpp"
#include "utility/sampler.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN

    class ImageTexture2D:public Texture2D{
    public:
        ImageTexture2D(const RC<Image2D<Color3b>>& image)
        :data(image)
        {}

        int width() const noexcept {
            return data->width();
        };

        int height() const noexcept {
            return data->height();
        }

        Spectrum evaluate_impl(const Point2f& uv) const noexcept override{
            auto v =  LinearSampler::Sample2D(*data.get(),uv.x,uv.y);
            //todo convert color3b to color3f
            return Spectrum(v.x/real(255.0),v.y/real(255.0),v.z/real(255.0));
        }
    private:
        RC<Image2D<Color3b>> data;
    };

    RC<Texture2D> create_image_texture2d(const RC<Image2D<Color3b>>& image){
        return newRC<ImageTexture2D>(image);
    }




TRACER_END