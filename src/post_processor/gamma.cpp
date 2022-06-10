//
// Created by wyz on 2022/6/10.
//
#include "core/post_processor.hpp"

TRACER_BEGIN


class GammaCorrector : public PostProcessor
{
    real gamma = 1;

public:

    explicit GammaCorrector(real gamma)
    :gamma(gamma)
    {
        if(gamma <= 0){
            LOG_ERROR("invalid gamma {} and reset to 1",gamma);
            this->gamma = 1;
        }

    }

    void process(RenderTarget &render_target) override
    {
        auto &image = render_target.color;
        for(int y = 0; y < image.height(); ++y)
        {
            for(int x = 0; x < image.width(); ++x)
            {
                image(x, y) = image(x, y).map([g = gamma](real c)->real
                                              {
                                                  return std::pow(c, g);
                                              });
            }
        }
    }
};

    RC<PostProcessor> create_gamma_corrector(real gamma){
        return newRC<GammaCorrector>(gamma);
    }

TRACER_END