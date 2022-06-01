//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_PARALLEL_HPP
#define TRACER_PARALLEL_HPP

#include <thread>
#include <mutex>
#include <vector>
#include <optional>
#include "common.hpp"
#include "geometry.hpp"

TRACER_BEGIN

inline int actual_worker_count(int worker_count) noexcept
{
    if(worker_count <= 0)
        worker_count += static_cast<int>(std::thread::hardware_concurrency());
    return (std::max)(1, worker_count);
}

    template<typename F>
    void parallel_for_1d_grid(int thread_count, int width, int grid_size, F &&func)
    {
        const int task_count = (width + grid_size - 1) / grid_size;

        std::atomic<int> next_task = 0;

        auto task = [&](int thread_index)
        {
            for(;;)
            {
                const int task_idx = next_task++;
                if(task_idx >= task_count)
                    return;

                const int beg = task_idx * grid_size;
                const int end = (std::min)(beg + grid_size, width);

                if constexpr(
                        std::is_convertible_v<
                                decltype(func(thread_index, beg, end)), bool>)
                {
                    if(!func(thread_index, beg, end))
                        return;
                }
                else
                    func(thread_index, beg, end);
            }
        };

        std::vector<std::thread> threads;
        for(int i = 0; i < thread_count; i++){
            threads.emplace_back(task,i);
        }
        for(auto& t:threads){
            t.join();
        }
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

    template<typename T, typename Func>
    void parallel_forrange(T beg, T end, Func &&func, int worker_count = 0)
    {
        std::mutex it_mutex;
        T it = beg;
        auto next_item = [&]() -> std::optional<T>
        {
            std::lock_guard lk(it_mutex);
            if(it == end)
                return std::nullopt;
            return std::make_optional(it++);
        };

        std::mutex except_mutex;
        std::exception_ptr except_ptr = nullptr;

        worker_count = actual_worker_count(worker_count);

        auto worker_func = [&](int thread_index)
        {
            for(;;)
            {
                auto item = next_item();
                if(!item)
                    break;

                try
                {
                    func(thread_index, *item);
                }
                catch(...)
                {
                    std::lock_guard lk(except_mutex);
                    if(!except_ptr)
                        except_ptr = std::current_exception();
                }

                std::lock_guard lk(except_mutex);
                if(except_ptr)
                    break;
            }
        };

        std::vector<std::thread> workers;
        for(int i = 0; i < worker_count; ++i)
            workers.emplace_back(worker_func, i);

        for(auto &w : workers)
            w.join();

        if(except_ptr)
            std::rethrow_exception(except_ptr);
    }

TRACER_END

#endif //TRACER_PARALLEL_HPP
