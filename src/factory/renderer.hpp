//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_FACTORY_RENDERER_HPP
#define TRACER_FACTORY_RENDERER_HPP

#include "common.hpp"

TRACER_BEGIN

struct PTRendererParams{
    int worker_count = 0;
    int task_tile_size = 16;
    int spp = 1;
    int min_depth = 3;
    int max_depth = 10;
    int direct_light_sample_num = 1;
};

RC<Renderer> create_pt_renderer(const PTRendererParams& params);



TRACER_END

#endif //TRACER_FACTORY_RENDERER_HPP
