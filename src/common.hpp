//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_COMMON_HPP
#define TRACER_COMMON_HPP
#include <cassert>
#include <stdexcept>
#include <memory>

#include <glm/glm.hpp>

#define TRACER_BEGIN namespace tracer{
#define TRACER_END }

TRACER_BEGIN

class Scene;
class Camera;

class Filter;
class Sampler;
class Ray;
class Renderer;

using Spectrum = glm::vec3;

using Vector3i = glm::vec<3,int>;
using Vector2i = glm::vec<2,int>;
using real = float;

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


TRACER_END



#endif //TRACER_COMMON_HPP
