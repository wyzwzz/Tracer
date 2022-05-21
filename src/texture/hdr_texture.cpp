//
// Created by wyz on 2022/5/21.
//
#include "core/texture.hpp"
#include "utility/image.hpp"

TRACER_BEGIN

    class HDRTexture2D: public Texture2D{
    public:
        HDRTexture2D(const RC<Image2D<Color3f>>& image)
        {}
        ~HDRTexture2D() override {}

        int width() const noexcept {
            return data->width();
        };

        int height() const noexcept {
            return data->height();
        }

        Spectrum evaluate(const SurfaceIntersection& isect) const noexcept {
            return {};
        }

        Spectrum evaluate(const Point2f& uv) const noexcept {
            return {};
        }
    private:
        RC<Image2D<Color3f>> data;

    };

    RC<Texture2D> create_hdr_texture2d(const RC<Image2D<Color3f>>& image){
        return newRC<HDRTexture2D>(image);
    }

TRACER_END