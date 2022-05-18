//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_PARALLEL_HPP
#define TRACER_PARALLEL_HPP

#include <thread>
#include <mutex>
#include <vector>

#include "common.hpp"
#include "geometry.hpp"

TRACER_BEGIN

inline int actual_worker_count(int worker_count) noexcept
{
    if(worker_count <= 0)
        worker_count += static_cast<int>(std::thread::hardware_concurrency());
    return (std::max)(1, worker_count);
}

    //todo capture or handle exception
    template<typename F>
    void parallel_for_2d(int thread_count,
                         int width,int height,
                         int tile_size_x,int tile_size_y,
                         F&& func){
        const int x_tile_count = (width + tile_size_x - 1) / tile_size_x;
        const int y_tile_count = (height + tile_size_y - 1) / tile_size_y;
        const int total_tile_count = x_tile_count * y_tile_count;

        thread_count = actual_worker_count(thread_count);

        std::atomic<int> next_tile = 0;

        auto task = [&](int thread_idx){
            for(;;){
                const int tile_idx = next_tile++;
                if(tile_idx >= total_tile_count)
                    return;

                const int tile_x_idx = tile_idx % x_tile_count;
                const int tile_y_idx = tile_idx / x_tile_count;

                const int tile_begin_x = tile_x_idx * tile_size_x;
                const int tile_begin_y = tile_y_idx * tile_size_y;
                const int tile_end_x = std::min(width,tile_begin_x + tile_size_x);
                const int tile_end_y = std::min(height,tile_begin_y + tile_size_y);

                Bounds2i tile_bound{{tile_begin_x,tile_begin_y},
                                    {tile_end_x,tile_end_y}};

                func(thread_idx,tile_bound);
            }

        };
        //todo using simple thread pool to replace
        std::vector<std::thread> threads;
        for(int i = 0; i < thread_count; i++){
            threads.emplace_back(task,i);
        }
        for(auto& t:threads){
            t.join();
        }
    }


TRACER_END

#endif //TRACER_PARALLEL_HPP
