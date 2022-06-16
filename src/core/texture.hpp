//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_TEXTURE_HPP
#define TRACER_TEXTURE_HPP
#include "utility/geometry.hpp"
#include "core/spectrum.hpp"
#include "core/intersection.hpp"
#include <algorithm>
#include "utility/logger.hpp"
TRACER_BEGIN

class Texture2D{
protected:
    using TextureWrapFunc = real(*)(real);
    static real wrap_clamp(real x) noexcept{
        return std::clamp<real>(x,0,1);
    }
    static real wrap_repeat(real x) noexcept{
        return std::clamp<real>(x-std::floor(x),0,1);
    }
    TextureWrapFunc wrapper_u = &wrap_repeat;
    TextureWrapFunc wrapper_v = &wrap_repeat;
    virtual Spectrum evaluate_impl(const Point2f& uv) const noexcept = 0;
public:
    virtual ~Texture2D() = default;

    virtual int width() const noexcept = 0;

    virtual int height() const noexcept = 0;


    virtual real evaluate_s(const Point2f& uv) const noexcept{
        const real u = wrapper_u(uv.x);
        const real v = wrapper_v(uv.y);
        auto ret = evaluate_impl({u,v});
        if(inv_gamma != 1){
            for(int i=0;i<SPECTRUM_COMPONET_COUNT;++i)
                ret[i] = std::pow(ret[i],inv_gamma);
        }
        return ret.r;
    }

    virtual Spectrum evaluate(const Point2f& uv) const noexcept {
        const real u = wrapper_u(uv.x);
        const real v = wrapper_v(uv.y);
        auto ret = evaluate_impl({u,v});
        if(inv_gamma != 1){
            for(int i=0;i<SPECTRUM_COMPONET_COUNT;++i)
                ret[i] = std::pow(ret[i],inv_gamma);
        }
        return ret;
    }

    virtual Spectrum evaluate(const SurfaceIntersection& isect) const noexcept {
        return evaluate(isect.uv);
    }

    virtual real evaluate_s(const SurfaceIntersection& isect) const noexcept {
        return evaluate_s(isect.uv);
    }
protected:
    real inv_gamma = 1;
};


class Texture3D{
protected:
    using TextureWrapFunc = real(*)(real);
    static real wrap_clamp(real x) noexcept{
        return std::clamp<real>(x,0,1);
    }
    static real wrap_repeat(real x) noexcept{
        return std::clamp<real>(x-std::floor(x),0,1);
    }
    TextureWrapFunc wrapper_u = &wrap_repeat;
    TextureWrapFunc wrapper_v = &wrap_repeat;
    TextureWrapFunc wrapper_w = &wrap_repeat;

    virtual Spectrum evaluate_impl(const Point3f& uvw) const noexcept = 0;

public:
    virtual ~Texture3D() = default;

    virtual int width() const noexcept = 0;

    virtual int height() const noexcept = 0;

    virtual int depth() const noexcept = 0;

    virtual real evaluate_s(const Point3f& uvw) const noexcept{
        const real u = wrapper_u(uvw.x);
        const real v = wrapper_v(uvw.y);
        const real w = wrapper_w(uvw.z);
        auto ret = evaluate_impl({u,v,w});
        if(inv_gamma != 1){
            for(int i=0;i<SPECTRUM_COMPONET_COUNT;++i)
                ret[i] = std::pow(ret[i],inv_gamma);
        }
        return ret.r;
    }

    virtual Spectrum evaluate(const Point3f& uvw) const noexcept {
        const real u = wrapper_u(uvw.x);
        const real v = wrapper_v(uvw.y);
        const real w = wrapper_w(uvw.z);
        auto ret = evaluate_impl({u,v,w});
        if(inv_gamma != 1){
            for(int i=0;i<SPECTRUM_COMPONET_COUNT;++i)
                ret[i] = std::pow(ret[i],inv_gamma);
        }
        return ret;
    }

    virtual Spectrum max_spectrum() const noexcept = 0;

    virtual real max_real() const noexcept = 0;
protected:
    real inv_gamma = 1;
};


TRACER_END

#endif //TRACER_TEXTURE_HPP
