//
// Created by wyz on 2022/5/22.
//

#ifndef TRACER_AGGREGATE_BSDF_HPP
#define TRACER_AGGREGATE_BSDF_HPP

#include "core/bsdf.hpp"
#include "bxdf.hpp"
TRACER_BEGIN

    template<int MAX_BXDF_COUNT>
    class AggregateBSDF:public BSDF{
    public:
        real weights[MAX_BXDF_COUNT];
        const BXDF* bxdfs[MAX_BXDF_COUNT];
        int bxdfs_count = 0;

        Spectrum eval(const Vector3f& wi,const Vector3f& wo) const override;

        BSDFSampleResult sample(const Vector3f& wo,const Sample3&) const override;

        real pdf(const Vector3f& wi, const Vector3f& wo) const override;


    private:

    };

TRACER_END

#endif //TRACER_AGGREGATE_BSDF_HPP
