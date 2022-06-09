//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_BSDF_HPP
#define TRACER_BSDF_HPP

#include "utility/geometry.hpp"
#include "core/spectrum.hpp"
#include "utility/coordinate.hpp"
#include "core/sampling.hpp"

TRACER_BEGIN

struct BSDFSampleResult{
    Vector3f  wi;
    Spectrum  f;
    real      pdf = 0;
    bool      is_delta = false;

    bool is_valid() const{
        return wi && pdf > 0 && !f.is_back();
    }
};

class BSDF: public NoCopy{
public:
    virtual ~BSDF() = default;

    // wi and wo must be normalized
    virtual Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const = 0;

    virtual BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3&) const = 0;

    virtual real pdf(const Vector3f& wi, const Vector3f& wo) const = 0;

    virtual bool is_delta() const = 0;

    //used for photo mapping
    virtual bool has_diffuse() const = 0;

    virtual Spectrum get_albedo() const = 0;
};

class LocalBSDF: public BSDF{
public:
    LocalBSDF(const Coord& geometry_coord,const Coord& shading_coord)
    :geometry_coord(geometry_coord),shading_coord(shading_coord)
    {

    }

protected:

    bool cause_black_fringes(const Vector3f& w) const{
        bool g = geometry_coord.in_positive_z_hemisphere(w);
        bool s = geometry_coord.in_positive_z_hemisphere(w);
        return g ^ s;
    }

    bool cause_black_fringes(const Vector3f& w1,const Vector3f& w2) const{
        return cause_black_fringes(w1) | cause_black_fringes(w2);
    }

    Spectrum evaluate_black_fringes(const Vector3f& wi,const Vector3f& wo) const{
        if(geometry_coord.in_positive_z_hemisphere(wi) &
           geometry_coord.in_positive_z_hemisphere(wo))
            return get_albedo() * invPI_r;
        return Spectrum(0);
    }

    BSDFSampleResult sample_black_fringes(const Vector3f& wo,TransportMode mode,const Sample3& sample) const{
        if(!geometry_coord.in_positive_z_hemisphere(wo))
            return {};
        auto lwi = CosineSampleHemisphere({sample.u,sample.v});
        auto pdf = CosineHemispherePdf(lwi.z);
        if(pdf < eps)
            return {};
        auto wi = geometry_coord.local_to_global(lwi).normalize();
        real normal_corr = normal_correct_factor(geometry_coord,shading_coord,wi);
        Spectrum f = get_albedo() * invPI_r * normal_corr;

        return BSDFSampleResult{wi,f,pdf,false};
    }

    real pdf_black_fringes(const Vector3f& wi,const Vector3f& wo) const{
        if(cause_black_fringes(wi,wo))
            return 0;
        return CosineHemispherePdf(geometry_coord.global_to_local(wi).normalize().z);
    }


    Coord geometry_coord;
    Coord shading_coord;
};

TRACER_END

#endif //TRACER_BSDF_HPP
