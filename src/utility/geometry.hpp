//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_GEOMETRY_HPP
#define TRACER_GEOMETRY_HPP

#include "common.hpp"
TRACER_BEGIN

template<typename T>
class Point2{
public:
    Point2(T xx,T yy):x(xx),y(yy)
    {}

    template<typename U>
    explicit Point2(const Point2<U>& p){
        x = (T)p.x;
        y = (T)p.y;
    }

    bool operator==(const Point2<T> &p) const { return x == p.x && y == p.y; }
    bool operator!=(const Point2<T> &p) const { return x != p.x || y != p.y; }

    Point2 operator+(const Point2& p) const{
        return Point2(x+p.x,y+p.y);
    }
    Point2 operator+(T t) const{
        return Point2(x+t,y+t);
    }

    Point2 operator-(const Point2& p) const{
        return Point2(x-p.x,y-p.y);
    }
    Point2 operator-(T t) const{
        return Point2(x-t,y-t);
    }


    T x,y;
};



    using Point2i = Point2<int>;
    using Point2f = Point2<real>;

    template <typename T>
    Point2<T> Floor(const Point2<T> &p) {
        return Point2<T>(std::floor(p.x), std::floor(p.y));
    }

    template <typename T>
    Point2<T> Ceil(const Point2<T> &p) {
        return Point2<T>(std::ceil(p.x), std::ceil(p.y));
    }

    template <typename T>
    Point2<T> Min(const Point2<T> &p1, const Point2<T> &p2) {
        return Point2<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
    }

    template <typename T>
    Point2<T> Max(const Point2<T> &p1, const Point2<T> &p2) {
        return Point2<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
    }

template <typename T>
class Bounds2{
public:
    Bounds2(const Point2<T>& low,const Point2<T>& high)
    :low(low),high(high)
    {}
    template <typename U>
    explicit operator Bounds2<U>() const{
        return Bounds2<U>((Point2<U>)low,(Point2<U>)high);
    }

    bool operator==(const Bounds2<T> &b) const {
        return b.low == low && b.high == high;
    }
    bool operator!=(const Bounds2<T> &b) const {
        return b.low != low || b.high != high;
    }

    T area() const{
        Point2<T> d = high - low;
        return d.x * d.y;
    }

    Point2<T> low,high;
};

using Bounds2i = Bounds2<int>;
using Bounds2f = Bounds2<real>;

    template <typename T>
    Bounds2<T> Intersect(const Bounds2<T> &b1, const Bounds2<T> &b2) {

        return Bounds2<T>(Max(b1.low,b2.low),Min(b1.high,b2.high));
    }

    class Bounds2iIterator : public std::forward_iterator_tag {
    public:
        Bounds2iIterator(const Bounds2i &b, const Point2i &pt)
                : p(pt), bounds(&b) {}
        Bounds2iIterator operator++() {
            advance();
            return *this;
        }
        Bounds2iIterator operator++(int) {
            Bounds2iIterator old = *this;
            advance();
            return old;
        }
        bool operator==(const Bounds2iIterator &bi) const {
            return p == bi.p && bounds == bi.bounds;
        }
        bool operator!=(const Bounds2iIterator &bi) const {
            return p != bi.p || bounds != bi.bounds;
        }

        Point2i operator*() const { return p; }

    private:
        void advance() {
            ++p.x;
            if (p.x == bounds->high.x) {
                p.x = bounds->low.x;
                ++p.y;
            }
        }
        Point2i p;
        const Bounds2i *bounds;
    };

    inline Bounds2iIterator begin(const Bounds2i &b) {
        return Bounds2iIterator(b, b.low);
    }

    inline Bounds2iIterator end(const Bounds2i &b) {
        // Normally, the ending point is at the minimum x value and one past
        // the last valid y value.
        Point2i pEnd(b.low.x, b.high.y);
        // However, if the bounds are degenerate, override the end point to
        // equal the start point so that any attempt to iterate over the bounds
        // exits out immediately.
        if (b.low.x >= b.high.x || b.low.y >= b.high.y)
            pEnd = b.low;
        return Bounds2iIterator(b, pEnd);
    }
TRACER_END

#endif //TRACER_GEOMETRY_HPP
