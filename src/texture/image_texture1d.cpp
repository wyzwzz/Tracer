//
// Created by wyz on 2022/6/16.
//
#include "../core/texture.hpp"
#include "../utility/image.hpp"
#include "../core/spectrum.hpp"
#include "utility/sampler.hpp"
#include <cassert>

TRACER_BEGIN

//color3f or real
template<typename T>
class ImageTexture1D:public Texture1D{
public:
    ImageTexture1D();

    int width() const noexcept override{

    }

    Spectrum evaluate_impl(real u) const noexcept override{

    }
private:

};

TRACER_END