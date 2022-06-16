//
// Created by wyz on 2022/6/16.
//
#include "../core/medium.hpp"
#include "phase_function.hpp"
#include "../core/sampler.hpp"
#include "utility/memory.hpp"
TRACER_BEGIN

class HomogeneousMedium: public Medium{
public:
    HomogeneousMedium(const Spectrum& sigma_a,const Spectrum& sigma_s,real g,int max_scattering_count)
    :sigma_a(sigma_a),sigma_s(sigma_s),g(g),max_scattering_count(max_scattering_count)
    {
        this->sigma_t = sigma_a + sigma_s;
        assert(g > -1 && g < 1);
        this->g = std::clamp<real>(g,-1 + eps,1 - eps);
    }

    ~HomogeneousMedium() = default;

    int get_max_scattering_count() const noexcept override{
        return max_scattering_count;
    }

    Spectrum tr(const Point3f& a,const Point3f& b,Sampler& sampler) const noexcept override{
        Spectrum exp = -sigma_t * (real)(a - b).length();
        return Spectrum(std::exp(exp.r),std::exp(exp.g),std::exp(exp.b));
    }

    MediumSampleResult sample(const Point3f& a,const Point3f& b,Sampler& sampler,MemoryArena& arena,bool indirect_scattering) const override{
        if(!sigma_s){
            return {{},nullptr,tr(a,b,sampler)};//no scattering and just absorb
        }
        const auto [u,v] = sampler.sample2();
        int channel = std::min<int>(u * SPECTRUM_COMPONET_COUNT,SPECTRUM_COMPONET_COUNT - 1);
        real dist = -std::log(1 - v) / sigma_t[channel];
        real t_max = (b - a).length();
        const Vector3f dir = (b - a).normalize();
        bool sample_medium = dist < t_max;
        Spectrum tr;
        for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
            tr[i] = std::exp(-sigma_t[i] * std::min(dist,t_max));
        //todo ???
        Spectrum density = sample_medium ? sigma_s * tr : tr;

        real pdf = 0;
        for(int i = 0; i < SPECTRUM_COMPONET_COUNT; i++)
            pdf += density[i];
        pdf /= SPECTRUM_COMPONET_COUNT;
        pdf = std::max(pdf,eps);

        Spectrum throughout = tr / pdf;
        if(sample_medium)
            throughout *= sigma_s;

        if(sample_medium){
            MediumScatteringP p;
            p.pos = a + dist * dir;
            p.medium = this;
            p.wo = -dir;
            auto phase_func = arena.alloc_object<HenyeyGreensteinPhaseFunction>(g,get_albedo());
            return MediumSampleResult{p,phase_func,throughout};
        }
        return MediumSampleResult{{},nullptr,throughout};
    }
private:
    Spectrum get_albedo() const{
        return !sigma_t ? Spectrum(1) : sigma_s / sigma_t;
    }
private:
    Spectrum sigma_a;
    Spectrum sigma_s;
    Spectrum sigma_t;
    real g = 0;
    int max_scattering_count = 0;
};


RC<Medium> create_homogeneous_medium(
        const Spectrum &sigma_a,
        const Spectrum &sigma_s,
        real g,
        int max_scattering_count){
    return newRC<HomogeneousMedium>(sigma_a,sigma_s,g,max_scattering_count);
}

TRACER_END