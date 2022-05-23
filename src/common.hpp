//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_COMMON_HPP
#define TRACER_COMMON_HPP
#include <cassert>
#include <stdexcept>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#define TRACER_BEGIN namespace tracer{
#define TRACER_END }

TRACER_BEGIN

class Scene;
class Camera;
class Shape;
class Aggregate;
class Material;
class Filter;
class Sampler;
class Ray;
class Renderer;
class Primitive;
class MediumInterface;
class BSDF;
class SurfaceIntersection;
class Light;
class EnvironmentLight;

    using real = float;

//using Spectrum = glm::vec3;


    constexpr real eps = real(3e-4);

    template<typename T>
    constexpr std::enable_if_t<std::is_floating_point_v<T>, T>
            PI = T(3.141592653589793238462643383);

    template<typename T>
    constexpr T invPI = 1 / PI<T>;

    template<typename T>
    constexpr T inv2PI = 1 / (2 * PI<T>);

    template<typename T>
    constexpr T inv4PI = 1 / (4 * PI<T>);

    template<typename T>
    constexpr T PIOver2 = PI<T> / 2;

    template<typename T>
    constexpr T PIOver4 = PI<T> / 4;


    constexpr real PI_r = PI<real>;
    constexpr real invPI_r = invPI<real>;
    constexpr real PIOver2_r = PIOver2<real>;
    constexpr real PIOver4_r = PIOver4<real>;

//A non-inline const variable that is not explicitly declared extern has internal linkage
constexpr real REAL_MAX = std::numeric_limits<real>::max();

    struct Sample1 { real u; };
    struct Sample2 { real u, v; };
    struct Sample3 { real u, v, w; };
    struct Sample4 { real u, v, w, r; };
    struct Sample5 { real u, v, w, r, s; };

    template<int N>
    struct SampleN { real u[N]; };


// smart pointers

    template<typename T>
    using RC = std::shared_ptr<T>;

    template<typename T, typename...Args>
    RC<T> newRC(Args&&...args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    RC<T> toRC(T &&data)
    {
        return newRC<T>(std::forward<T>(data));
    }

    template<typename T>
    using Box = std::unique_ptr<T>;

    template<typename T, typename...Args>
    Box<T> newBox(Args&&...args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Span = std::vector<T>;

    //

    template <typename T>
    inline constexpr bool IsPowerOf2(T v) {
        return v && !(v & (v - 1));
    }



TRACER_END



#endif //TRACER_COMMON_HPP
