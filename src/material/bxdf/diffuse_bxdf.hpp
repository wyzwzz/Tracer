//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_DIFFUSE_COMP_HPP
#define TRACER_DIFFUSE_COMP_HPP

#include "bxdf.hpp"

TRACER_BEGIN

    class DiffuseBXDF : public BXDF{
    private:
        Spectrum coef; // albedo / pi
    public:
        explicit DiffuseBXDF(const Spectrum& albedo);

        ~DiffuseBXDF() override = default;

        Spectrum evaluate(const Vector3f& lwi,const Vector3f& lwo,TransportMode mode) const override;

        real pdf(const Vector3f& lwi,const Vector3f& lwo) const override;

        BXDFSampleResult sample(const Vector3f& lwo,TransportMode mode,const Sample2& sample) const override;

        bool has_diffuse() const override;
    };

TRACER_END

#endif //TRACER_DIFFUSE_COMP_HPP
