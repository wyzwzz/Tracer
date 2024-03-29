//
// Created by wyz on 2022/5/19.
//

#ifndef TRACER_MEMORY_HPP
#define TRACER_MEMORY_HPP
#include <list>
#include "common.hpp"

TRACER_BEGIN

#define ARENA_ALLOC(arena, T) new ((arena).alloc(sizeof(T))) T

void* alloc_aligned(size_t size);

void free_aligned(void*);

//no construct
template <typename T>
T* alloc_aligned(size_t count){
    return (T*) alloc_aligned(count * sizeof(T));
}

    inline constexpr size_t L1_CACHE_LINE_SIZE = 64;
    //todo add args for construct
    class alignas(L1_CACHE_LINE_SIZE) MemoryArena{
    public:
        MemoryArena(size_t max_block_size = 262144):max_block_size(max_block_size){}
        MemoryArena(const MemoryArena &) = delete;
        MemoryArena &operator=(const MemoryArena &) = delete;
        ~MemoryArena() {
            free_aligned(current_block);
            for (auto &block : used_blocks) free_aligned(block.second);
            for (auto &block : available_blocks) free_aligned(block.second);
        }
        void* alloc(size_t nBytes){
            return _alloc(nBytes);
        }
        void* _alloc(size_t nBytes){
            const int align = alignof(std::max_align_t);
            static_assert(IsPowerOf2(align), "Minimum alignment not a power of two");
            nBytes = (nBytes + align - 1) & ~(align - 1);
            if(current_block_pos + nBytes > current_alloc_size){
                //alloc new block
                if(current_block){
                    used_blocks.emplace_back(current_alloc_size,current_block);
                    current_block = nullptr;
                    current_alloc_size = 0;
                }
                for(auto it = available_blocks.begin(); it != available_blocks.end();it++){
                    if(it->first >= nBytes){
                        current_alloc_size = it->first;
                        current_block = it->second;
                        available_blocks.erase(it);
                        break;
                    }
                }
                if(!current_block){
                    current_alloc_size = std::max(nBytes,max_block_size);
                    current_block = alloc_aligned<uint8_t>(current_alloc_size);
                }
                current_block_pos = 0;
            }
            void* ret = current_block + current_block_pos;
            current_block_pos += nBytes;
            return ret;
        }
        template <typename T>
        T *alloc(size_t n = 1, bool run_constructor = true) {
            T *ret = (T *)_alloc(n * sizeof(T));
            if (run_constructor)
                for (size_t i = 0; i < n; ++i) new (&ret[i]) T();
            return ret;
        }

        template<typename T,typename... Args>
        T* alloc_object(Args&&... args){
            T *ret = (T*)_alloc(sizeof(T));
            new (ret) T(std::forward<Args>(args)...);
            return ret;
        }

        void reset() {
            current_block_pos = 0;
            available_blocks.splice(available_blocks.begin(), used_blocks);
        }
        size_t total_allocated() const {
            size_t total = current_alloc_size;
            for (const auto &alloc : used_blocks) total += alloc.first;
            for (const auto &alloc : available_blocks) total += alloc.first;
            return total;
        }
        size_t used_bytes() const{
            size_t used = 0;
            for(const auto& alloc:used_blocks) used += alloc.first;
            return used;
        }

    private:
        const size_t max_block_size;
        size_t current_block_pos = 0, current_alloc_size = 0;
        uint8_t *current_block = nullptr;
        //todo available with priority queue or sort after every erase/insert
        std::list<std::pair<size_t, uint8_t *>> used_blocks, available_blocks;
    };

    /**
 * @brief 浮点数原子加法
 */
    template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    void atomic_add(std::atomic<T> &original, T add_val)
    {
        T cur_val = original.load(std::memory_order_consume);
        T new_val = cur_val + add_val;
        while(!original.compare_exchange_weak(
                cur_val, new_val, std::memory_order_release, std::memory_order_consume))
        {
            new_val = cur_val + add_val;
        }
    }

TRACER_END

#endif //TRACER_MEMORY_HPP
