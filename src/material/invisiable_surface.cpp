#include "../core/bsdf.hpp"
#include "../core/material.hpp"
#include "../core/bssrdf.hpp"

TRACER_BEGIN

class InvisibleSurfaceBSDF:public BSDF{
public:
    explicit InvisibleSurfaceBSDF(const Vector3f& gn)
    :gn(gn)
    {}

    Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
        return {};
    }

    BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3&) const override{
        real cos_theta = abs_cos(gn,wo);
        Spectrum f = Spectrum(1) / (cos_theta < eps ? 1 : cos_theta);
        return {-wo,f,1,true};
    }

    real pdf(const Vector3f& wi, const Vector3f& wo) const override{
        return 0;
    }

    bool is_delta() const override{
        return true;
    }

    bool has_diffuse() const override{
        return false;
    }

    Spectrum get_albedo() const override{
        return Spectrum(1);
    }
private:
    Vector3f gn;
};

class InvisibleSurface final: public Material{
    RC<const BSSRDFSurface> bssrdf_surface;
public:
    InvisibleSurface(RC<const BSSRDFSurface> bssrdf_surface)
    :bssrdf_surface(std::move(bssrdf_surface))
    {}

    InvisibleSurface(){}

    Spectrum evaluate(const Point2f& uv) const  override{
        return Spectrum(1);
    }

    SurfaceShadingPoint shading(const SurfaceIntersection& isect,MemoryArena& arena) const override{
        SurfaceShadingPoint shading_pt;
        shading_pt.bsdf = arena.alloc_object<InvisibleSurfaceBSDF>(isect.geometry_coord.z);
        shading_pt.shading_n = isect.shading_coord.z;
        if(bssrdf_surface)
            shading_pt.bssrdf = bssrdf_surface->create(isect,arena);
        return shading_pt;
    }

};

RC<Material> create_invisible_surface(RC<const BSSRDFSurface> bssrdf_surface){
    return newRC<InvisibleSurface>(std::move(bssrdf_surface));
}

RC<Material> create_invisible_surface(){
    return newRC<InvisibleSurface>();
}

TRACER_END