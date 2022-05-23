//
// Created by wyz on 2022/5/21.
//
#include "core/texture.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN


    class ConstantTexture2D:public Texture2D{
    public:
        ConstantTexture2D(const Spectrum& constant)
        :constant(constant)
        {}

        int width() const noexcept {
            return 1;
        };

        int height() const noexcept {
            return 1;
        }

        Spectrum evaluate_impl(const Point2f& uv) const noexcept override{
            return constant;
        }
    private:
        Spectrum constant;
    };


    RC<Texture2D> create_constant_texture2d(const Spectrum& constant){
        return newRC<ConstantTexture2D>(constant);
    }



TRACER_END