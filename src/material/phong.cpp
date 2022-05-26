//
// Created by wyz on 2022/5/20.
//
#include "core/material.hpp"
#include "core/texture.hpp"
#include "core/intersection.hpp"
#include "utility/logger.hpp"
#include "aggregate_bsdf.hpp"
#include "bxdf/diffuse_bxdf.hpp"
#include "bxdf/phong_specular_bxdf.hpp"
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

        SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const override{
            SurfaceShadingPoint shading_p;
            Spectrum diffuse = map_kd->evaluate(isect.uv);
            Spectrum specular = map_ks->evaluate(isect.uv);
            const real ns = map_ns->evaluate_s(isect.uv);

            // ensure energy conservation

            real dem = 1;
            for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                dem = (std::max)(dem, diffuse[i] + specular[i]);
            diffuse /= dem;
            specular /= dem;

            const real diffuse_lum = diffuse.lum();
            const real specular_lum = specular.lum();



            real diffuse_weight;
            if(diffuse_lum + specular_lum > eps)
                diffuse_weight = diffuse_lum / (diffuse_lum + specular_lum);
            else{
                diffuse_weight = real(0.5);
            }



            const real specular_weight = 1 - diffuse_weight;

            auto bsdf = arena.alloc_object<AggregateBSDF<2>>(isect);

            bsdf->add_bxdf(arena.alloc_object<DiffuseBXDF>(diffuse),diffuse_weight);

            bsdf->add_bxdf(arena.alloc_object<PhongSpecularBXDF>(specular,ns),specular_weight);

            shading_p.bsdf = bsdf;
            //todo normal map

            return shading_p;
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