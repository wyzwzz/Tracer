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
            T *ret = (T *)alloc(n * sizeof(T));
            if (run_constructor)
                for (size_t i = 0; i < n; ++i) new (&ret[i]) T();
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

    private:
        const size_t max_block_size;
        size_t current_block_pos = 0, current_alloc_size = 0;
        uint8_t *current_block = nullptr;
        std::list<std::pair<size_t, uint8_t *>> used_blocks, available_blocks;
    };

TRACER_END

#endif //TRACER_MEMORY_HPP
