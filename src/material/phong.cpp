//
// Created by wyz on 2022/5/20.
//
#include "core/material.hpp"
#include "core/texture.hpp"
#include "core/intersection.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN


    class Phong:public Material{
    public:
        Phong(RC<const Texture2D> map_ka,
              RC<const Texture2D> map_kd,
              RC<const Texture2D> map_ks,
              RC<const Texture2D> map_ns)
        :map_ka(map_ka),map_kd(map_kd),map_ks(map_ks),map_ns(map_ns)
        {}

        ~Phong() override {}

        virtual Spectrum evaluate(const Point2f& uv) const {
            return map_kd->evaluate(uv);
        }

        virtual SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const{
            return {};
        }
    private:
        RC<const Texture2D> map_ka;
        RC<const Texture2D> map_kd;
        RC<const Texture2D> map_ks;
        RC<const Texture2D> map_ns;
    };



    RC<Material> create_phong_material(
            RC<const Texture2D> map_ka,
            RC<const Texture2D> map_kd,
            RC<const Texture2D> map_ks,
            RC<const Texture2D> map_ns
    ){
        return newRC<Phong>(
                map_ka,
                map_kd,
                map_ks,
                map_ns
                );
    }


TRACER_END