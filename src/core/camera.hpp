//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_CAMERA_HPP
#define TRACER_CAMERA_HPP
#include "common.hpp"
#include "utility/geometry.hpp"
#include "core/spectrum.hpp"
TRACER_BEGIN
struct CameraSample {
    Point2f p_film;//相对于整个film分辨率的坐标 范围在[0~1]
    Point2f pr_lens;
};

struct CameraEvalWeResult{
    Spectrum we;
    Point2f film_coord;
    Normal3f n;
};

struct CameraPdfWeResult{
    real pdf_pos;
    real pdf_dir;
};

struct CameraSampleWiResult{
    Point3f pos_on_cam;
    Normal3f n;
    Vector3f ref_to_pos;//ref to pos on camera
    Spectrum we;
    real pdf = 0;
    Point2f film_coord;
};

class Camera{
public:
    virtual ~Camera() = default;

    virtual real generate_ray(const CameraSample&,Ray&) const noexcept = 0;

    virtual CameraEvalWeResult eval_we(const Point3f& pos_on_cam,const Vector3f& pos_to_out) const noexcept = 0;

    virtual CameraPdfWeResult pdf_we(const Point3f& pos_on_cam,const Vector3f& pos_to_out) const noexcept = 0;

    virtual CameraSampleWiResult sample_wi(const Point3f& ref,const Sample2& sample) const noexcept = 0;
};

TRACER_END

#endif //TRACER_CAMERA_HPP
