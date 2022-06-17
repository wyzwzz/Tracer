//
// Created by wyz on 2022/6/16.
//
#include "../core/medium.hpp"
#include "phase_function.hpp"
#include "../core/sampler.hpp"
#include "utility/memory.hpp"
#include "../core/texture.hpp"
TRACER_BEGIN

class Volume:public Medium{
public:
    virtual int get_max_scattering_count() const noexcept = 0;

    virtual Spectrum tr(const Point3f& a,const Point3f& b,Sampler& sampler) const noexcept = 0;

    virtual MediumSampleResult sample(const Point3f& a,const Point3f& b,Sampler& sampler,MemoryArena& arena,bool indirect_scattering = false) const = 0;

private:
    Transform world_to_local;
    RC<const Texture3D> scale;
    RC<const Texture1D> albedo;
    RC<const Texture1D> alpha;
    real max_density;
    real inv_max_density;//control ray step
    int max_scattering_count;
    bool white_for_indirect;
};

TRACER_END