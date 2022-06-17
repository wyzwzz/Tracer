//
// Created by wyz on 2022/6/17.
//

#ifndef TRACER_CORE_BSSRDF_HPP
#define TRACER_CORE_BSSRDF_HPP

#include "intersection.hpp"
#include "spectrum.hpp"
TRACER_BEGIN

struct BSSRDFSampleResult{
    SurfaceIntersection isect;
    Spectrum coef;
    real pdf = 0;
    bool invalid() const{
        return pdf == 0 && coef.is_back();
    }
};
class BSSRDF:public NoCopy{
public:
    virtual ~BSSRDF() = default;

    virtual BSSRDFSampleResult sample_pi(const Sample3& sample,MemoryArena& arena) const = 0;
};

class BSSRDFSurface{
public:
    virtual ~BSSRDFSurface() = default;

    virtual BSSRDF* create(const SurfaceIntersection& isect,MemoryArena& arena) const{
        return nullptr;
    }
};

TRACER_END

#endif //TRACER_CORE_BSSRDF_HPP
