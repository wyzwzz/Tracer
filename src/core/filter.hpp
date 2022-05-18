//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_FILTER_HPP
#define TRACER_FILTER_HPP

#include "common.hpp"

TRACER_BEGIN

    class Filter{
    public:
        virtual ~Filter() = default;

        virtual real radius() const noexcept = 0;

        virtual real eval(real x,real y) const noexcept = 0;
    };

TRACER_END


#endif //TRACER_FILTER_HPP
