//
// Created by wyz on 2022/5/18.
//
#include "core/filter.hpp"

TRACER_BEGIN


    class GaussianFilter:public Filter{
    public:
        GaussianFilter(real radius,real alpha)
        :r(radius),alpha(alpha),exp(std::exp(-alpha * r * r))
        {

        }
        real radius() const noexcept{
            return r;
        }

        real eval(real x,real y) const noexcept{
            return gaussian(x,exp) * gaussian(y,exp);
        }
    private:
        real r;
        real alpha;
        real exp;

        real gaussian(float d,float expv) const{
            return std::max((real)0,real(std::exp(-alpha*d*d)-expv));
        }
    };

    RC<Filter> create_gaussin_filter(real radius,real alpha){
        return newRC<GaussianFilter>(radius,alpha);
    }

TRACER_END