//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_SAMPLER_HPP
#define TRACER_SAMPLER_HPP

#include <chrono>
#include <random>

#include <pcg_random.hpp>
#include "utility/memory.hpp"
#include "common.hpp"

TRACER_BEGIN

class Sampler{
public:
    virtual ~Sampler() = default;

    virtual Sample1 sample1() = 0;
    virtual Sample2 sample2() = 0;
    virtual Sample3 sample3() = 0;
    virtual Sample4 sample4() = 0;
    virtual Sample5 sample5() = 0;

    virtual Box<Sampler> clone(int seed)  = 0;
};

    class SimpleUniformSampler: public Sampler{
    public:
        using rng_t = pcg32;// std::mt19937_64;
        using seed_t = rng_t::result_type;

        SimpleUniformSampler(int seed,bool use_time = false)
        :seed(seed),rng(seed)
        {}

        virtual Sample1 sample1(){
            return { distribution(rng) };
        }
        virtual Sample2 sample2(){
            const real u = sample1().u;
            const real v = sample1().u;
            return { u, v };
        }
        virtual Sample3 sample3(){
            const real u = sample1().u;
            const real v = sample1().u;
            const real w = sample1().u;
            return { u, v, w };
        }
        virtual Sample4 sample4(){
            const real u = sample1().u;
            const real v = sample1().u;
            const real w = sample1().u;
            const real r = sample1().u;
            return { u, v, w, r };
        }
        virtual Sample5 sample5(){
            const real u = sample1().u;
            const real v = sample1().u;
            const real w = sample1().u;
            const real r = sample1().u;
            const real s = sample1().u;
            return { u, v, w, r, s };
        }

        virtual Box<Sampler> clone(int _seed){
            std::seed_seq::result_type new_seed;
            {
                std::seed_seq seed_gen = {
                        std::seed_seq::result_type(seed),
                        std::seed_seq::result_type(_seed)
                };
                seed_gen.generate(&new_seed, &new_seed + 1);
            }
            return newBox<SimpleUniformSampler>(seed);
        }

        seed_t get_seed() const noexcept {return seed;}
    private:
        seed_t seed;
        rng_t rng;
        std::uniform_real_distribution<real> distribution;
    };

    class PerThreadNativeSamplers
    {
    public:

        PerThreadNativeSamplers();

        PerThreadNativeSamplers(size_t threadCount, const SimpleUniformSampler &parent);

        PerThreadNativeSamplers(PerThreadNativeSamplers &&other) noexcept;

        PerThreadNativeSamplers &operator=(PerThreadNativeSamplers &&other) noexcept;

        ~PerThreadNativeSamplers();

        void swap(PerThreadNativeSamplers &other) noexcept;

        SimpleUniformSampler *get_sampler(size_t threadIdx) noexcept;

        SimpleUniformSampler *operator[](size_t threadIdx) noexcept;

    private:

        static constexpr size_t STORAGE_ALIGN = 64;
        static constexpr size_t STORAGE_SIZE  = 64;

        struct SamplerStorage
        {
            SamplerStorage(int seed, bool use_time_seed);

            SimpleUniformSampler sampler;
            char cache_pad[STORAGE_SIZE - sizeof(SimpleUniformSampler)] = {};
        };

        static_assert(sizeof(SamplerStorage) == STORAGE_SIZE);

        size_t count_;
        SamplerStorage *samplers_;
    };

    inline PerThreadNativeSamplers::PerThreadNativeSamplers()
            : count_(0), samplers_(nullptr)
    {

    }

    inline PerThreadNativeSamplers::PerThreadNativeSamplers(
            size_t threadCount, const SimpleUniformSampler &parent)
            : count_(threadCount)
    {
        samplers_ = reinterpret_cast<SamplerStorage *>(
                alloc_aligned(sizeof(SamplerStorage) * threadCount));

        size_t i = 0;
        try
        {
            for(; i < threadCount; ++i)
            {
                new(samplers_ + i) SamplerStorage(
                        static_cast<int>(parent.get_seed() + i), false);
            }
        }
        catch(...)
        {
            free_aligned(samplers_);
            throw;
        }
    }

    inline PerThreadNativeSamplers::PerThreadNativeSamplers(
            PerThreadNativeSamplers &&other) noexcept
            : PerThreadNativeSamplers()
    {
        swap(other);
    }

    inline PerThreadNativeSamplers &PerThreadNativeSamplers::operator=(
            PerThreadNativeSamplers &&other) noexcept
    {
        swap(other);
        return *this;
    }

    inline PerThreadNativeSamplers::~PerThreadNativeSamplers()
    {
        if(!samplers_)
            return;
        free_aligned(samplers_);
    }

    inline void PerThreadNativeSamplers::swap(PerThreadNativeSamplers &other) noexcept
    {
        std::swap(count_, other.count_);
        std::swap(samplers_, other.samplers_);
    }

    inline SimpleUniformSampler *PerThreadNativeSamplers::get_sampler(
            size_t threadIdx) noexcept
    {
        return &samplers_[threadIdx].sampler;
    }

    inline SimpleUniformSampler *PerThreadNativeSamplers::operator[](
            size_t threadIdx) noexcept
    {
        return get_sampler(threadIdx);
    }

    inline PerThreadNativeSamplers::SamplerStorage::SamplerStorage(
            int seed, bool use_time_seed)
            : sampler(seed, use_time_seed)
    {

    }

TRACER_END

#endif //TRACER_SAMPLER_HPP
