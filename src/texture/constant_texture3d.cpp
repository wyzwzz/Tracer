//
// Created by wyz on 2022/6/16.
//
#include "../core/texture.hpp"
#include "../utility/image.hpp"

TRACER_BEGIN


class Constant3D:public Texture3D{
public:
    Constant3D(const Spectrum& constant)
    :constant(constant)
    {}

    ~Constant3D() = default;

    int width() const noexcept override{
        return 1;
    }

    int height() const noexcept override{
        return 1;
    }

    int depth() const noexcept override{
        return 1;
    }

    Spectrum evaluate_impl(const Point3f& uvw) const noexcept override{
        return constant;
    }

    Spectrum max_spectrum() const noexcept override{
        return constant;
    }

    real max_real() const noexcept override{
        return constant.r;
    }

private:
    Spectrum constant;
};


RC<Texture3D> create_constant_texture3d(const Spectrum& constant){
    return newRC<Constant3D>(constant);
}

TRACER_END