//
// Created by wyz on 2022/5/23.
//

#ifndef TRACER_DIRECT_ILLUMINATION_HPP
#define TRACER_DIRECT_ILLUMINATION_HPP
#include "core/light.hpp"
#include "core/bsdf.hpp"
#include "core/intersection.hpp"
#include "core/sampler.hpp"
#include "utility/distribution.hpp"
TRACER_BEGIN

Spectrum sample_light(const Scene& scene,const Light* light,
                      const SurfaceIntersection& isect,
                      const SurfaceShadingPoint& shd_p,
                      Sampler& sampler);

Spectrum sample_light(const Scene& scene,const Light* light,
                      const MediumScatteringP& scattering_p,
                      const BSDF* phase_func,
                      Sampler& sampler);


Spectrum sample_area_light(const Scene& scene,const AreaLight* light,
                           const SurfaceIntersection& isect,
                           const SurfaceShadingPoint& shd_p,
                           Sampler& sampler);

Spectrum sample_environment_light(const Scene& scene,const EnvironmentLight* light,
                                  const SurfaceIntersection& isect,
                                  const SurfaceShadingPoint& shd_p,
                                  Sampler& sampler);

Spectrum sample_area_light(const Scene& scene,const AreaLight* light,
                      const MediumScatteringP& scattering_p,
                      const BSDF* phase_func,
                      Sampler& sampler);

Spectrum sample_environment_light(const Scene& scene,const EnvironmentLight* light,
                      const MediumScatteringP& scattering_p,
                      const BSDF* phase_func,
                      Sampler& sampler);

Spectrum sample_bsdf(const Scene& scene,const SurfaceIntersection& isect,
                     const SurfaceShadingPoint& shd_p,Sampler& sampler);

Spectrum sample_bsdf(const Scene& scene,const MediumScatteringP& scattering_p,
                     const BSDF* phase_func,Sampler& sampler);

Box<Distribution1D> compute_light_power_distribution(const Scene& scene);

TRACER_END
#endif //TRACER_DIRECT_ILLUMINATION_HPP
