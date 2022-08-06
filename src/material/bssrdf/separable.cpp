#include "separable.hpp"
#include "core/sampling.hpp"
#include <list>
#include "core/scene.hpp"
#include "utility/memory.hpp"
#include "core/bsdf.hpp"
#include "core/material.hpp"
#include "utility/reflection.hpp"
TRACER_BEGIN

namespace {
    real fresnel_moment(real eta){
        const real eta2 = eta * eta;
        const real eta3 = eta2 * eta;
        const real eta4 = eta2 * eta2;
        const real eta5 = eta2 * eta3;

        if(eta < 1)
        {
            return real(0.45966)
                   - real(1.73965)  * eta
                   + real(3.37668)  * eta2
                   - real(3.904945) * eta3
                   + real(2.49277)  * eta4
                   - real(0.68441)  * eta5;
        }

        return real(-4.61686)
               + real(11.1136) * eta
               - real(10.4646) * eta2
               + real(5.11455) * eta3
               - real(1.27198) * eta4
               + real(0.12746) * eta5;
    }

    class SeparableBSDF: public BSDF{
    public:
        SeparableBSDF(const Coord& coord,real eta)
        :coord(coord),eta(eta)
        {}

        Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
            const real cos_theta_i = cos(wi,coord.z);
            real c_i = 1 - 2 * fresnel_moment(1 / eta);

            real fr = dielectric_fresnel(eta,1,cos_theta_i);
            real val = (1 - fr) / (c_i * PI_r);
            return Spectrum(val) * (eta * eta);
        }

        BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sample) const override{
            auto local_dir = CosineSampleHemisphere({sample.u,sample.v});
            auto pdf = CosineHemispherePdf(local_dir.z);
            auto dir = coord.global_to_local(local_dir);
            return BSDFSampleResult{dir,eval(dir,wo,mode),pdf,false};
        }

        real pdf(const Vector3f& wi, const Vector3f& wo) const override{
            return CosineHemispherePdf(coord.global_to_local(wi).normalize().z);
        }

        bool is_delta() const override{
            return false;
        }

        bool has_diffuse() const override{
            return true;
        }

        Spectrum get_albedo() const override{
            return {};
        }
    private:
        Coord coord;
        real eta;
    };

    class SeparableBSDFMaterial:public Material{
    public:
        explicit SeparableBSDFMaterial(const BSDF* bsdf)
        :bsdf(bsdf)
        {

        }
        Spectrum evaluate(const Point2f& uv) const override{
            return {};
        }
        SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const override{
            SurfaceShadingPoint shading_point;
            shading_point.shading_n = isect.geometry_coord.z;
            shading_point.bsdf = bsdf;
            shading_point.bssrdf = nullptr;
            return shading_point;
        }
    private:
        const BSDF* bsdf;
    };
}

SeparableBSSRDF::SeparableBSSRDF(const SurfaceIntersection &po, real eta)
:po(po),eta(eta)
{

}

BSSRDFSampleResult SeparableBSSRDF::sample_pi(const Scene& scene,const Sample3 &sample, MemoryArena &arena) const {
    auto [channel,sample_u] = extract_uniform_int(sample.u,0,SPECTRUM_COMPONET_COUNT);

    Coord proj_coord;
    real sample_v;
    if(sample.v < 0.5){
        proj_coord = po.geometry_coord;
        sample_v = 2 * sample.v;
    }
    else if(sample.v < 0.75){
        proj_coord = Coord(po.geometry_coord.y,po.geometry_coord.z,po.geometry_coord.z);
        sample_v = 4 * (sample.v - real(0.5));
    }
    else{
        proj_coord = Coord(po.geometry_coord.z,po.geometry_coord.x,po.geometry_coord.y);
        sample_v = 4 * (sample.v - real(0.75));
    }

    const auto sr = sample_r(channel,{sample_u});
    const real r_max = sample_r(channel,{real(0.996)}).dist;//996!!!
    if(sr.dist <= 0 || sr.dist > r_max)
        return {};
    const real phi = 2 * PI_r * sample_v;

    // ray
    const real half_l = std::sqrt(std::max(real(0),r_max * r_max - sr.dist * sr.dist));
    Vector3f isect_ray_offset(sr.dist * std::cos(phi),sr.dist * std::sin(phi),half_l);//local
    real isect_ray_len = 2 * half_l;

    Ray isect_ray(po.pos + proj_coord.local_to_global(isect_ray_offset),-proj_coord.z,eps,std::max(eps,isect_ray_len));

    //not use std::list because we will use arena to alloc memory
    struct IsectListNode{
        SurfaceIntersection isect;
        IsectListNode* next;
    }* isect_list = nullptr;

    int isect_count = 0;
    do{
        SurfaceIntersection new_isect;
        if(!scene.intersect_p(isect_ray,&new_isect))
            break;

        if(new_isect.material == po.material)//may use primitive?
        {
            auto node = arena.alloc_object<IsectListNode>();
            node->isect = new_isect;
            node->next = isect_list;
            isect_list = node;
            isect_count++;
        }

        isect_ray.o = isect_ray(isect_ray.t_max + eps);

        if(isect_ray.t_max >= isect_ray_len)
            break;

    }while(true);

    if(isect_count == 0)
        return {};

    auto [isect_index,sample_w] = extract_uniform_int(sample.w,0,isect_count);
    for(int i = 0; i < isect_index; ++i){
        assert(isect_list);
        isect_list = isect_list->next;
    }
    assert(isect_list);
    assert(isect_list->isect.material == po.material);

    const real pdf_r = pdf_pi(isect_list->isect);

    const BSDF* bsdf = arena.alloc_object<SeparableBSDF>(isect_list->isect.geometry_coord,eta);

    SurfaceIntersection isect = isect_list->isect;
    isect.material = arena.alloc_object<SeparableBSDFMaterial>(bsdf);

    real cos_theta_o = cos(po.wo,po.geometry_coord.z);
    real fro = 1 - dielectric_fresnel(eta,1,cos_theta_o);

    Spectrum coef = fro * eval_r(distance(isect.pos,po.pos));
    real pdf = pdf_r / (2 * PI_r) / isect_count;

    return BSSRDFSampleResult{isect,coef,pdf};
}

real SeparableBSSRDF::pdf_pi(const SurfaceIntersection &pi) const {
    const Vector3f ld = po.geometry_coord.global_to_local(po.pos - pi.pos);
    const Vector3f ln = po.geometry_coord.global_to_local(pi.geometry_coord.z);
    const real r_proj[] = {
            static_cast<real>(Vector2f(ld.y, ld.z).length()),
            static_cast<real>(Vector2f(ld.z, ld.x).length()),
            static_cast<real>(Vector2f(ld.x, ld.y).length())
    };

    constexpr real AXIS_PDF[] = { real(0.25), real(0.25), real(0.5) };
    constexpr real CHANNEL_PDF = real(1) / SPECTRUM_COMPONET_COUNT;

    real ret = 0;
    for(int axis_idx = 0; axis_idx < 3; ++axis_idx)
    {
        for(int ch_idx = 0; ch_idx < SPECTRUM_COMPONET_COUNT; ++ch_idx)
        {
            ret += std::abs(ln[axis_idx]) *
                   pdf_r(ch_idx, r_proj[axis_idx]) * AXIS_PDF[axis_idx];
        }
    }

    return ret * CHANNEL_PDF;
}

TRACER_END