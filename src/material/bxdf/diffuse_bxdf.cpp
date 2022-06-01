//
// Created by wyz on 2022/5/23.
//
#include "diffuse_bxdf.hpp"
#include "core/sampling.hpp"
TRACER_BEGIN

    DiffuseBXDF::DiffuseBXDF(const Spectrum& albedo)
            :coef(albedo/PI_r)
    {

    }

    Spectrum DiffuseBXDF::evaluate(const Vector3f &local_wi, const Vector3f &local_wo) const {
        if(local_wi.z <= 0 || local_wo.z <= 0)
            return {};
        return coef;
    }

    BXDFSampleResult DiffuseBXDF::sample(const Vector3f &wo, const Sample2 &sample) const {
        if(wo.z <= 0) return {};
        BXDFSampleResult ret;
        auto lwi = CosineSampleHemisphere(sample);
        auto pdf = CosineHemispherePdf(lwi.z);
        if(pdf < real(0.001))
            return {};

        ret.lwi = lwi;
        ret.pdf = pdf;
        ret.f = coef;
        return ret;
    }

    real DiffuseBXDF::pdf(const Vector3f &lwi, const Vector3f &lwo) const {
        if(lwi.z <= 0 || lwo.z <= 0)
            return 0;
        return CosineHemispherePdf(lwi.z);
    }

    bool DiffuseBXDF::has_diffuse() const {
        return true;
    }

TRACER_END