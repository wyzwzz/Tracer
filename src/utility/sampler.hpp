//
// Created by wyz on 2022/5/21.
//

#ifndef TRACER_SAMPLER_HPP
#define TRACER_SAMPLER_HPP

#include "image.hpp"
#include <algorithm>
TRACER_BEGIN


    struct LinearSampler
    {

        template <typename Texel>
        static auto Sample2D(const Image2D<Texel> &tex, double u, double v) -> Texel
        {
            u = std::clamp(u, 0.0, 1.0) * (tex.width() - 1);
            v = std::clamp(v, 0.0, 1.0) * (tex.height() - 1);
            int u0 = std::clamp(static_cast<int>(u), 0, static_cast<int>(tex.width() - 1));
            int u1 = std::clamp(u0 + 1, 0, static_cast<int>(tex.width() - 1));
            int v0 = std::clamp(static_cast<int>(v), 0, static_cast<int>(tex.height() - 1));
            int v1 = std::clamp(v0 + 1, 0, static_cast<int>(tex.height() - 1));
            double d_u = u - u0;
            double d_v = v - v0;
            return (tex(u0, v0) * (1.0 - d_u) + tex(u1, v0) * d_u) * (1.0 - d_v) +
                   (tex(u0, v1) * (1.0 - d_u) + tex(u1, v1) * d_u) * d_v;
        }

        template <typename Texel>
        static auto Sample3D(const Image3D<Texel> &tex, double u, double v, double k) -> Texel
        {
            u = std::clamp(u, 0.0, 1.0) * (tex.GetXSize() - 1);
            v = std::clamp(v, 0.0, 1.0) * (tex.GetYSize() - 1);
            k = std::clamp(k, 0.0, 1.0) * (tex.GetZSize() - 1);
            int u0 = std::clamp(static_cast<int>(u), 0, static_cast<int>(tex.GetXSize() - 1));
            int u1 = std::clamp(u0 + 1, 0, static_cast<int>(tex.GetXSize() - 1));
            int v0 = std::clamp(static_cast<int>(v), 0, static_cast<int>(tex.GetYSize() - 1));
            int v1 = std::clamp(v0 + 1, 0, static_cast<int>(tex.GetYSize() - 1));
            int k0 = std::clamp(static_cast<int>(k), 0, static_cast<int>(tex.GetZSize() - 1));
            int k1 = std::clamp(k0 + 1, 0, static_cast<int>(tex.GetZSize() - 1));
            double d_u = u - u0;
            double d_v = v - v0;
            double d_k = k - k0;
            return ((tex(u0, v0, k0) * (1.0 - d_u) + tex(u1, v0, k0) * d_u) * (1.0 - d_v) +
                    (tex(u0, v1, k0) * (1.0 - d_u) + tex(u1, v1, k0) * d_u) * d_v) *
                   (1.0 - d_k) +
                   ((tex(u0, v0, k1) * (1.0 - d_u) + tex(u1, v0, k1) * d_u) * (1.0 - d_v) +
                    (tex(u0, v1, k1) * (1.0 - d_u) + tex(u1, v1, k1) * d_u) * d_v) *
                   d_k;
        }

    };

TRACER_END

#endif //TRACER_SAMPLER_HPP
