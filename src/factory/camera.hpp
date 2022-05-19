//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_FACTORY_CAMERA_HPP
#define TRACER_FACTORY_CAMERA_HPP
#include "common.hpp"
TRACER_BEGIN
RC<Camera> create_thin_lens_camera(real film_aspect,
                                   const Point3f& cam_pos,const Point3f& target,const Vector3f& up,
                                   real fov,real lens_radius,real focal_dist);
TRACER_END
#endif //TRACER_FACTORY_CAMERA_HPP
