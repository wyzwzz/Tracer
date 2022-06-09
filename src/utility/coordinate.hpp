//
// Created by wyz on 2022/6/9.
//

#ifndef TRACER_COORDINATE_HPP
#define TRACER_COORDINATE_HPP

#include "geometry.hpp"
#include "transform.hpp"

TRACER_BEGIN

using float3 = Vector3f;
//todo replace float3 with simd float3
class coord_float3_t{
public:
    using axis_t = float3;
    using vec_t  = float3;
    using self_t = coord_float3_t;

    axis_t x, y, z;

    coord_float3_t();

    coord_float3_t(const axis_t& x,const axis_t& y,const axis_t& z);

    static self_t create_from_z(const axis_t& new_z);

    vec_t global_to_local(const vec_t& global_vec) const;

    vec_t local_to_global(const vec_t& local_vec) const;

    self_t rotate_to_new_z(const axis_t& new_z) const;

    bool in_positive_z_hemisphere(const vec_t& v) const;

};

using float3_coord = coord_float3_t;

using Coord = float3_coord;

inline real normal_correct_factor(const Vector3f& g,const Vector3f& s,const Vector3f& wi)
{
    real dem = std::abs(cos(g,wi));
    return dem < eps ? 1 : std::abs(cos(s,wi)) / dem;
}

inline real normal_correct_factor(const Coord& g,const Coord& s,const Vector3f& wi)
{
    return normal_correct_factor(g.z,s.z,wi);
}

 inline void compute_ss_ts(const Vector3f& AB,const Vector3f& AC,
                         const Vector2f& ab,const Vector2f& ac,const Vector3f& n,
                         Vector3f& ss,Vector3f& ts){
    const real m00 = ab.x, m01 = ab.y;
    const real m10 = ac.x, m11 = ac.y;
    const real det = m00 * m11 - m01 * m10;
    if(det){
        const real inv_det = 1 / det;
        ss  = m11 * inv_det * AB - m01 * inv_det * AC;
        ts = m10 * inv_det * AB - m00 * inv_det * AC;
    }
    else{
        coordinate(n,ss,ts);
    }
}


//todo move impl to .inl
inline coord_float3_t::coord_float3_t()
:x(axis_t(1,0,0)),y(axis_t(0,1,0)),z(axis_t(0,0,1))
{

}

inline coord_float3_t::coord_float3_t(const axis_t &x, const axis_t &y, const axis_t &z)
:x(x.normalize()),y(y.normalize()),z(z.normalize())
{

}

inline coord_float3_t coord_float3_t::create_from_z(const axis_t &z)
{
    auto new_z = z.normalize();
    axis_t new_y;
    if(std::abs(new_z.x) > std::abs(new_z.y))
        new_y = vec_t(-new_z.z, 0, new_z.x).normalize();
    else
        new_y = vec_t(0,new_z.z,-new_z.y).normalize();
    return self_t(cross(new_y,new_z),new_y,new_z);
}

inline coord_float3_t::vec_t coord_float3_t::global_to_local(const vec_t &global_vec) const
{
    return vec_t(dot(x,global_vec),dot(y,global_vec),dot(z,global_vec));
}

inline coord_float3_t::vec_t coord_float3_t::local_to_global(const vec_t &local_vec) const
{
    return x * local_vec.x + y * local_vec.y + z * local_vec.z;
}

inline coord_float3_t coord_float3_t::rotate_to_new_z(const axis_t &new_z) const
{
    //order x y z
    axis_t new_x = cross(y,new_z);
    axis_t new_y = cross(new_z,new_x);
    return self_t(new_x,new_y,new_z);
}

inline bool coord_float3_t::in_positive_z_hemisphere(const vec_t &v) const
{
    return dot(v,z) > 0;
}

TRACER_END

#endif //TRACER_COORDINATE_HPP
