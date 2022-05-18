//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_SAMPLER_HPP
#define TRACER_SAMPLER_HPP

#include <chrono>
#include <random>

#include <pcg_random.hpp>

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

        SimpleUniformSampler(int seed)
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

    private:
        seed_t seed;
        rng_t rng;
        std::uniform_real_distribution<real> distribution;
    };

TRACER_END

#endif //TRACER_SAMPLER_HPP
