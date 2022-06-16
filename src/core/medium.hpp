//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_MEDIUM_HPP
#define TRACER_MEDIUM_HPP

#include "common.hpp"
#include "spectrum.hpp"
#include "intersection.hpp"
#include "utility/memory.hpp"
TRACER_BEGIN

struct MediumSampleResult{


    MediumScatteringP scattering_point;

    const BSDF* phase_func = nullptr;

    Spectrum throughout;

    bool is_scattering_happened() const noexcept{
        return phase_func != nullptr;
    }
};
static_assert(sizeof(MediumSampleResult) == 56,"");

class Medium{
public:
    virtual ~Medium() = default;

    virtual int get_max_scattering_count() const noexcept = 0;

    virtual Spectrum tr(const Point3f& a,const Point3f& b,Sampler& sampler) const noexcept = 0;

    virtual MediumSampleResult sample(const Point3f& a,const Point3f& b,Sampler& sampler,MemoryArena& arena,bool indirect_scattering = false) const = 0;

};

struct MediumInterface{
    RC<const Medium> inside;
    RC<const Medium> outside;
};
static_assert(sizeof(MediumInterface) == 32,"");

TRACER_END

#endif //TRACER_MEDIUM_HPP
