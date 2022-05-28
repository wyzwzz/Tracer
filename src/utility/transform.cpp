//
// Created by wyz on 2022/5/19.
//
#include "transform.hpp"
#include "utility/logger.hpp"
namespace tracer{
    Matrix4x4::Matrix4x4(real mat[4][4]) { memcpy(m, mat, 16 * sizeof(real)); }

    Matrix4x4::Matrix4x4(real t00, real t01, real t02, real t03, real t10,
                         real t11, real t12, real t13, real t20, real t21,
                         real t22, real t23, real t30, real t31, real t32,
                         real t33) {
        m[0][0] = t00;
        m[0][1] = t01;
        m[0][2] = t02;
        m[0][3] = t03;
        m[1][0] = t10;
        m[1][1] = t11;
        m[1][2] = t12;
        m[1][3] = t13;
        m[2][0] = t20;
        m[2][1] = t21;
        m[2][2] = t22;
        m[2][3] = t23;
        m[3][0] = t30;
        m[3][1] = t31;
        m[3][2] = t32;
        m[3][3] = t33;
    }

    Matrix4x4 transpose(const Matrix4x4 &m){
        return Matrix4x4(m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0], m.m[0][1],
                         m.m[1][1], m.m[2][1], m.m[3][1], m.m[0][2], m.m[1][2],
                         m.m[2][2], m.m[3][2], m.m[0][3], m.m[1][3], m.m[2][3],
                         m.m[3][3]);
    }

    Matrix4x4 inverse(const Matrix4x4 &m){
        int indxc[4], indxr[4];
        int ipiv[4] = {0, 0, 0, 0};
        real minv[4][4];
        memcpy(minv, m.m, 4 * 4 * sizeof(real));
        for (int i = 0; i < 4; i++) {
            int irow = 0, icol = 0;
            real big = 0.f;
            // Choose pivot
            for (int j = 0; j < 4; j++) {
                if (ipiv[j] != 1) {
                    for (int k = 0; k < 4; k++) {
                        if (ipiv[k] == 0) {
                            if (std::abs(minv[j][k]) >= big) {
                                big = real(std::abs(minv[j][k]));
                                irow = j;
                                icol = k;
                            }
                        } else if (ipiv[k] > 1)
                            LOG_ERROR("Singular matrix in MatrixInvert");
                    }
                }
            }
            ++ipiv[icol];
            // Swap rows _irow_ and _icol_ for pivot
            if (irow != icol) {
                for (int k = 0; k < 4; ++k) std::swap(minv[irow][k], minv[icol][k]);
            }
            indxr[i] = irow;
            indxc[i] = icol;
            if (minv[icol][icol] == 0.f){
                LOG_ERROR("Singular matrix in MatrixInvert");
            }

            // Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
            real pivinv = 1. / minv[icol][icol];
            minv[icol][icol] = 1.;
            for (int j = 0; j < 4; j++) minv[icol][j] *= pivinv;

            // Subtract this row from others to zero out their columns
            for (int j = 0; j < 4; j++) {
                if (j != icol) {
                    real save = minv[j][icol];
                    minv[j][icol] = 0;
                    for (int k = 0; k < 4; k++) minv[j][k] -= minv[icol][k] * save;
                }
            }
        }
        // Swap columns to reflect permutation
        for (int j = 3; j >= 0; j--) {
            if (indxr[j] != indxc[j]) {
                for (int k = 0; k < 4; k++)
                    std::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
            }
        }
        return Matrix4x4(minv);
    }

    Transform look_at(const Point3f& pos,const Point3f& target,const Vector3f& up){
        Matrix4x4 camera_to_world;
        camera_to_world.m[0][3] = pos.x;
        camera_to_world.m[1][3] = pos.y;
        camera_to_world.m[2][3] = pos.z;
        camera_to_world.m[3][3] = 1;

        Vector3f dir = normalize(target - pos);
        //注意是左手系
        Vector3f right = normalize(cross(normalize(up),dir));
        Vector3f new_up = cross(dir,right);
        //第一列是x 第二列是y 第三列是z 分别对应right up dir
        //第四列是pos
        camera_to_world.m[0][0] = right.x;
        camera_to_world.m[1][0] = right.y;
        camera_to_world.m[2][0] = right.z;
        camera_to_world.m[3][0] = 0.0;
        camera_to_world.m[0][1] = new_up.x;
        camera_to_world.m[1][1] = new_up.y;
        camera_to_world.m[2][1] = new_up.z;
        camera_to_world.m[3][1] = 0.0;
        camera_to_world.m[0][2] = dir.x;
        camera_to_world.m[1][2] = dir.y;
        camera_to_world.m[2][2] = dir.z;
        camera_to_world.m[3][2] = 0.0;
        auto inv = inverse(camera_to_world);
        auto m = Matrix4x4::mul(camera_to_world,inv);
        return Transform(inverse(camera_to_world),camera_to_world);
    }


    Transform rotate_x(real theta) {
        real sinTheta = std::sin(theta);
        real cosTheta = std::cos(theta);
        Matrix4x4 m(1, 0, 0, 0, 0, cosTheta, -sinTheta, 0, 0, sinTheta, cosTheta, 0,
                    0, 0, 0, 1);
        return Transform(m, transpose(m));
    }

    Transform rotate_y(real theta){
        real sinTheta = std::sin(theta);
        real cosTheta = std::cos(theta);
        Matrix4x4 m(cosTheta, 0, sinTheta, 0, 0, 1, 0, 0, -sinTheta, 0, cosTheta, 0,
                    0, 0, 0, 1);
        return Transform(m, transpose(m));
    }

    Transform rotate_z(real theta){
        real sinTheta = std::sin(theta);
        real cosTheta = std::cos(theta);
        Matrix4x4 m(cosTheta, -sinTheta, 0, 0, sinTheta, cosTheta, 0, 0, 0, 0, 1, 0,
                    0, 0, 0, 1);
        return Transform(m, transpose(m));
    }
}