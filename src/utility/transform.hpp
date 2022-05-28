//
// Created by wyz on 2022/5/19.
//

#ifndef TRACER_TRANSFORM_HPP
#define TRACER_TRANSFORM_HPP

#include "utility/geometry.hpp"

namespace tracer{

    struct Matrix4x4 {
        Matrix4x4() {
            m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
            m[0][1] = m[0][2] = m[0][3] = m[1][0] = m[1][2] = m[1][3] = m[2][0] =
            m[2][1] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.f;
        }
        Matrix4x4(real mat[4][4]);
        Matrix4x4(real t00, real t01, real t02, real t03, real t10, real t11,
                  real t12, real t13, real t20, real t21, real t22, real t23,
                  real t30, real t31, real t32, real t33);
        bool operator==(const Matrix4x4 &m2) const {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    if (m[i][j] != m2.m[i][j]) return false;
            return true;
        }
        bool operator!=(const Matrix4x4 &m2) const {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    if (m[i][j] != m2.m[i][j]) return true;
            return false;
        }
        friend Matrix4x4 transpose(const Matrix4x4 &);

        static Matrix4x4 mul(const Matrix4x4 &m1, const Matrix4x4 &m2) {
            Matrix4x4 r;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    r.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] +
                                m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
            return r;
        }
        friend Matrix4x4 inverse(const Matrix4x4 &);


        real m[4][4];
    };

    class Transform{
    public:
        Transform() {}
        Transform(const real mat[4][4]) {
            m = Matrix4x4(mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0],
                          mat[1][1], mat[1][2], mat[1][3], mat[2][0], mat[2][1],
                          mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2],
                          mat[3][3]);
            inv_m = inverse(m);//call friend function
        }
        Transform(const Matrix4x4 &m) : m(m), inv_m(inverse(m)) {}
        Transform(const Matrix4x4 &m, const Matrix4x4 &inv_m) : m(m), inv_m(inv_m) {}

        friend Transform inverse(const Transform &t) {
            return Transform(t.inv_m, t.m);
        }
        friend Transform transpose(const Transform &t) {
            return Transform(transpose(t.m), transpose(t.inv_m));
        }

        template<typename T>
        Point3<T> operator()(const Point3<T>& p) const{
            T x = p.x, y = p.y, z = p.z;
            T xp = m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z + m.m[0][3];
            T yp = m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z + m.m[1][3];
            T zp = m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z + m.m[2][3];
            T wp = m.m[3][0] * x + m.m[3][1] * y + m.m[3][2] * z + m.m[3][3];
            if (wp == 1)
                return Point3<T>(xp, yp, zp);
            else
                return Point3<T>(xp, yp, zp) / wp;
        }
        template<typename T>
        Normal3<T> operator()(const Normal3<T>& n) const{
            T x = n.x, y = n.y, z = n.z;
            return Normal3<T>(inv_m.m[0][0] * x + inv_m.m[1][0] * y + inv_m.m[2][0] * z,
                              inv_m.m[0][1] * x + inv_m.m[1][1] * y + inv_m.m[2][1] * z,
                              inv_m.m[0][2] * x + inv_m.m[1][2] * y + inv_m.m[2][2] * z);
        }
        template<typename T>
        Vector3<T> operator()(const Vector3<T>& v) const{
            T x = v.x, y = v.y, z = v.z;
            return Vector3<T>(m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z,
                              m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z,
                              m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z);
        }

    private:
        Matrix4x4 m,inv_m;
    };

    Transform translate(const Vector3f& delta);

    Transform scale(real x,real y,real z);

    //rad
    Transform rotate_x(real theta);

    Transform rotate_y(real theta);

    Transform rotate_z(real theta);

    Transform rotate(real theta, const Vector3f& axis);

    Transform look_at(const Point3f& pos,const Point3f& target,const Vector3f& up);


}


#endif //TRACER_TRANSFORM_HPP
