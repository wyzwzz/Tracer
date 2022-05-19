//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_CAMERA_HPP
#define TRACER_CAMERA_HPP
#include "common.hpp"
#include "utility/geometry.hpp"

TRACER_BEGIN
struct CameraSample {
    Point2f p_film;//相对于整个film分辨率的坐标 范围在[0~1]
    Point2f pr_lens;
};

class Camera{
public:
    virtual ~Camera() = default;

    virtual real generate_ray(const CameraSample&,Ray&) const noexcept = 0;
};

TRACER_END

#endif //TRACER_CAMERA_HPP
