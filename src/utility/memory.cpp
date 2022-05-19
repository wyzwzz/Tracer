//
// Created by wyz on 2022/5/19.
//
#include "memory.hpp"
TRACER_BEGIN

    void* alloc_aligned(size_t size){
        return _aligned_malloc(size,L1_CACHE_LINE_SIZE);
    }

    void free_aligned(void* ptr){
        if(ptr){
            _aligned_free(ptr);
        }
    }

TRACER_END