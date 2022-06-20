//
// Created by wyz on 2022/5/22.
//

#ifndef TRACER_SPECTRUM_HPP
#define TRACER_SPECTRUM_HPP

#include "common.hpp"
#include "utility/geometry.hpp"
TRACER_BEGIN

    template<typename T>
    class Color3{
    public:

    T r,g,b;

     Color3(): Color3(0){}
     Color3(T r,T g,T b):r(r),g(g),b(b){}
     explicit Color3(T val):Color3(val,val,val){}

     //rgb == 0
     bool is_back() const noexcept{
         return r == 0 && g == 0 && b == 0;
     }

     //rgb >= 0 and is finite
     bool is_valid() const noexcept{
         return r >=0 && g >= 0 && b >= 0 && is_finite();
     }

     //rgb > 0 and is finite
     bool is_meaningful() const noexcept{
         return is_finite() && r > 0 && g > 0 && b > 0;
     }

     bool is_finite() const noexcept{
         return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
     }

     bool operator!() const{
         return is_back();
     }

     Color3& operator+=(const Color3& rhs){
         r += rhs.r;
         g += rhs.g;
         b += rhs.b;
         return *this;
     }

     Color3& operator/=(T t){
         r /= t;
         g /= t;
         b /= t;
         return *this;
     }

        Color3& operator*=(T t){
            r *= t;
            g *= t;
            b *= t;
            return *this;
        }

        Color3& operator*=(const Color3& t){
            r *= t.r;
            g *= t.g;
            b *= t.b;
            return *this;
        }

     T& operator[](int idx){
         assert(idx<3 && idx >= 0);
         if(idx == 0) return r;
         else if(idx == 1) return g;
         else return b;
     }

      const T& operator[](int idx) const{
        assert(idx<3 && idx >= 0);
        if(idx == 0) return r;
        else if(idx == 1) return g;
        else return b;
    }

     T max_component_value(){
         T v = (*this)[0];
         for(int i = 1; i < 3; i++)
             v = std::max(v,(*this)[i]);
         return v;
     }

     T lum() const{
         return 0.2126f * r + 0.7152f * g + 0.0722f * b;
     }

     template<typename F>
     auto map(F && f) const noexcept{
         using RT = std::remove_cv_t<decltype(f(r))>;
         return Color3<RT>(f(r),f(g),f(b));
     }

    };

    template<typename T>
    Color3<T> operator-(const Color3<T>& lhs){
        return Color3<T>(-lhs.r,-lhs.g,-lhs.b);
    }

    template<typename T>
    Color3<T> operator+(const Color3<T>& lhs,const Color3<T>& rhs){
        return Color3<T>(lhs.r+rhs.r,lhs.g+rhs.g,lhs.b+rhs.b);
    }

    template<typename T>
    Color3<T> operator-(const Color3<T>& lhs,const Color3<T>& rhs){
        return Color3<T>(lhs.r-rhs.r,lhs.g-rhs.g,lhs.b-rhs.b);
    }

    template<typename T>
    Color3<T> operator*(const Color3<T>& lhs,const Color3<T>& rhs){
        return Color3<T>(lhs.r*rhs.r,lhs.g*rhs.g,lhs.b*rhs.b);
    }

    template<typename T>
    Color3<T> operator/(const Color3<T>& lhs,const Color3<T>& rhs){
        return Color3<T>(lhs.r/rhs.r,lhs.g/rhs.g,lhs.b/rhs.b);
    }

    template<typename T>
    Color3<T> operator*(const Color3<T>& lhs,const T& rhs){
        return Color3<T>(lhs.r*rhs,lhs.g*rhs,lhs.b*rhs);
    }

    template<typename T>
    Color3<T> operator*(const T& lhs,const Color3<T>& rhs){
        return rhs * lhs;
    }

    template<typename T>
    Color3<T> operator/(const Color3<T>& lhs,const T& rhs){
        return Color3<T>(lhs.r/rhs,lhs.g/rhs,lhs.b/rhs);
    }

    template<typename T>
    Color3<T> sqrt(const Color3<T>& c){
        return Color3<T>(std::sqrt(c.r),std::sqrt(c.g),std::sqrt(c.b));
    }

    template<typename T>
    Color3<T> exp(const Color3<T>& c){
        return Color3<T>(std::exp(c.r),std::exp(c.g),std::exp(c.b));
    }

    using Spectrum = Color3<real>;
    constexpr int SPECTRUM_COMPONET_COUNT = 3;

TRACER_END

#endif //TRACER_SPECTRUM_HPP
