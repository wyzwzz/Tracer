//
// Created by wyz on 2022/6/10.
//
#include "core/post_processor.hpp"

TRACER_BEGIN

class ACESToneMapper: public PostProcessor{
    static real aces_curve(real x) noexcept
    {
        constexpr real tA = real(2.51);
        constexpr real tB = real(0.03);
        constexpr real tC = real(2.43);
        constexpr real tD = real(0.59);
        constexpr real tE = real(0.14);
        return std::clamp(
                (x * (tA * x + tB)) / (x * (tC * x + tD) + tE), real(0), real(1));
    }
    real exposure = 1;
public:
    explicit ACESToneMapper(real exposure)
    :exposure(exposure)
    {
        if(exposure < 0){
            LOG_ERROR("invalid exposure : {} and reset to 1",exposure);
            this->exposure = 1;
        }
    }

    void process(RenderTarget &render_target) override
    {
        //todo parallel accelerate
        auto &image = render_target.color;
        for(int y = 0; y < image.height(); ++y)
        {
            for(int x = 0; x < image.width(); ++x)
            {
                auto &pixel = image(x, y);
                pixel.r = aces_curve(pixel.r * exposure);
                pixel.g = aces_curve(pixel.g * exposure);
                pixel.b = aces_curve(pixel.b * exposure);
            }
        }
    }
};


    RC<PostProcessor> create_aces_tone_mapper(real exposure){
        return newRC<ACESToneMapper>(exposure);
    }

TRACER_END