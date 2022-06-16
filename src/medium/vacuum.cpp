//
// Created by wyz on 2022/6/16.
//
#include "../core/medium.hpp"

TRACER_BEGIN

class VacuumMedium:public Medium{
public:
    ~VacuumMedium() = default;

    int get_max_scattering_count() const noexcept override{
        return std::numeric_limits<int>::max();
    }

    Spectrum tr(const Point3f& a,const Point3f& b,Sampler& sampler) const noexcept override{
        return Spectrum(1);
    }

    MediumSampleResult sample(const Point3f& a,const Point3f& b,Sampler& sampler,MemoryArena& arena,bool indirect_scattering = false) const override{
        return {{},nullptr,Spectrum(1)};
    }

};

RC<Medium> create_vacuum(){
    static RC<Medium> vacuum = newRC<VacuumMedium>();
    return vacuum;
}

TRACER_END