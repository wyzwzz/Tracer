//
// Created by wyz on 2022/6/16.
//

#ifndef TRACER_FACTORY_MEDIUM_HPP
#define TRACER_FACTORY_MEDIUM_HPP

#include "../core/medium.hpp"

TRACER_BEGIN


RC<Medium> create_heterogeneous_medium(
        const Transform &local_to_world,
        RC<const Texture3D> density,
        RC<const Texture3D> albedo,
        RC<const Texture3D> g,
        int max_scattering_count,
        bool white_for_indirect);

RC<Medium> create_homogeneous_medium(
        const Spectrum &sigma_a,
        const Spectrum &sigma_s,
        real g,
        int max_scattering_count);

RC<Medium> create_vacuum();


TRACER_END

#endif //TRACER_FACTORY_MEDIUM_HPP
