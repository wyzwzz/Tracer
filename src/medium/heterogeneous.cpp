//
// Created by wyz on 2022/6/16.
//
#include "../core/medium.hpp"
#include "phase_function.hpp"
#include "../core/sampler.hpp"
#include "utility/memory.hpp"
#include "core/texture.hpp"
TRACER_BEGIN

class HeterogeneousMedium:public Medium{
public:
    HeterogeneousMedium(const Transform& local_to_world,
                        RC<const Texture3D> density,
                        RC<const Texture3D> albedo,
                        RC<const Texture3D> g,
                        int max_scattering_count,
                        bool white_for_indirect)
                        :local_to_world(local_to_world),world_to_local(inverse(local_to_world)),
                        density(std::move(density)),
                        albedo(std::move(albedo)),
                        g(std::move(g)),
                        max_scattering_count(max_scattering_count),
                        white_for_indirect(white_for_indirect)
                        {
        this->max_density = density->max_real();
        this->max_density = std::max(this->max_density,eps);
        this->inv_max_density = 1 / this->max_density;
    }

    int get_max_scattering_count() const noexcept override{
        return max_scattering_count;
    }

    Spectrum tr(const Point3f& a,const Point3f& b,Sampler& sampler) const noexcept override{
        real res = 1;
        real t = 0;
        real t_max = (b - a).length();
        Vector3f dir = (b - a).normalize();

        do{
            real delta = - std::log(1- sampler.sample1().u) * inv_max_density;
            t += delta;
            if(t >= t_max)
                break;

            Point3f pos = a + dir * t;
            Point3f local_pos = world_to_local(pos);
            real _density = density->evaluate_s(local_pos);
            res *= 1 - _density * inv_max_density;
        } while (true);
        return Spectrum(res);
    }

    MediumSampleResult sample(const Point3f& a,const Point3f& b,Sampler& sampler,MemoryArena& arena,bool indirect_scattering = false) const override{
        const real t_max = (a-b).length();
        const Vector3f dir = (b - a).normalize();
        real t = 0;
        do{
            real delta_t = - std::log(1 - sampler.sample1().u) * inv_max_density;
            t += delta_t;
            if(t >= t_max)
                break;
            Point3f pos = a + dir * t;
            Point3f local_pos = world_to_local(pos);
            real _density = density->evaluate_s(local_pos);
            if(sampler.sample1().u < _density * inv_max_density){
                Spectrum _albedo = indirect_scattering && white_for_indirect ? Spectrum(1) : albedo->evaluate(local_pos);
                real _g = g->evaluate_s(local_pos);

                MediumScatteringP p ;
                p.pos = pos;
                p.medium = this;
                p.wo = -dir;

                auto phase_func = arena.alloc_object<HenyeyGreensteinPhaseFunction>(_g,_albedo);

                return MediumSampleResult{p,phase_func,_albedo};
            }
        } while (true);
        return MediumSampleResult{{},nullptr,Spectrum(1)};
    }


private:
    Transform local_to_world;
    Transform world_to_local;
    RC<const Texture3D> density;
    RC<const Texture3D> albedo;
    RC<const Texture3D> g;
    real max_density;
    real inv_max_density;
    int max_scattering_count;
    bool white_for_indirect;
};

RC<Medium> create_heterogeneous_medium(
        const Transform &local_to_world,
        RC<const Texture3D> density,
        RC<const Texture3D> albedo,
        RC<const Texture3D> g,
        int max_scattering_count,
        bool white_for_indirect){
    return newRC<HeterogeneousMedium>(local_to_world,density,albedo,g,max_scattering_count,white_for_indirect);
}

TRACER_END

