//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_GEOMETRY_HPP
#define TRACER_GEOMETRY_HPP

#include "common.hpp"
#include "hash.hpp"
namespace tracer {

    template<typename T>
    class Point2;

    template<typename T>
    class Point3;

    template <typename T>
    class Vector2 {
    public:
        // Vector2 Public Methods
        Vector2() { x = y = 0; }
        Vector2(T xx, T yy) : x(xx), y(yy) {}
        explicit Vector2(const Point2<T> &p):x(p.x),y(p.y){}
        explicit Vector2(const Point3<T> &p):x(p.x),y(p.y){}
#ifndef NDEBUG
        // The default versions of these are fine for release builds; for debug
        // we define them so that we can add the Assert checks.
        Vector2(const Vector2<T> &v) {
            x = v.x;
            y = v.y;
        }
        Vector2<T> &operator=(const Vector2<T> &v) {
            x = v.x;
            y = v.y;
            return *this;
        }
#endif  // !NDEBUG

        Vector2<T> operator+(const Vector2<T> &v) const {
            return Vector2(x + v.x, y + v.y);
        }

        Vector2<T> &operator+=(const Vector2<T> &v) {
            x += v.x;
            y += v.y;
            return *this;
        }
        Vector2<T> operator-(const Vector2<T> &v) const {
            return Vector2(x - v.x, y - v.y);
        }

        Vector2<T> &operator-=(const Vector2<T> &v) {
            x -= v.x;
            y -= v.y;
            return *this;
        }
        bool operator==(const Vector2<T> &v) const { return x == v.x && y == v.y; }
        bool operator!=(const Vector2<T> &v) const { return x != v.x || y != v.y; }
        template <typename U>
        Vector2<T> operator*(U f) const {
            return Vector2<T>(f * x, f * y);
        }

        template <typename U>
        Vector2<T> &operator*=(U f) {
            x *= f;
            y *= f;
            return *this;
        }
        template <typename U>
        Vector2<T> operator/(U f) const {
            double inv = (double)1 / f;
            return Vector2<T>(x * inv, y * inv);
        }

        template <typename U>
        Vector2<T> &operator/=(U f) {
            double inv = (double)1 / f;
            x *= inv;
            y *= inv;
            return *this;
        }
        Vector2<T> operator-() const { return Vector2<T>(-x, -y); }
        T operator[](int i) const {
            if (i == 0) return x;
            return y;
        }

        T &operator[](int i) {
            if (i == 0) return x;
            return y;
        }
        double length_squared() const { return x * x + y * y; }
        double length() const { return std::sqrt(length_squared()); }


        T x, y;
    };
    template <typename T, typename U>
    inline Vector2<T> operator*(U f, const Vector2<T> &v) {
        return v * f;
    }



    template<typename T>
    class Point2 {
    public:
        Point2(){}
        Point2(T xx, T yy) : x(xx), y(yy) {}

        template<typename U>
        explicit Point2(const Point2<U> &p) {
            x = (T) p.x;
            y = (T) p.y;
        }

        template<typename U>
        Point2(const Vector2<U>& v)
        :x(v.x),y(v.y)
        {}

        bool operator==(const Point2<T> &p) const { return x == p.x && y == p.y; }

        bool operator!=(const Point2<T> &p) const { return x != p.x || y != p.y; }

        Point2 operator+(const Point2 &p) const {
            return Point2(x + p.x, y + p.y);
        }

        Point2 operator+(T t) const {
            return Point2(x + t, y + t);
        }

        Point2<T> operator-(const Point2 &p) const {
            return Point2(x - p.x, y - p.y);
        }



        Point2 operator-(T t) const {
            return Point2(x - t, y - t);
        }

        template <typename U>
        Point2<T> operator*(U f) const {
            return Point2<T>(f * x, f * y);
        }

        Point2<T> operator-(const Vector2<T> &v) const {
            return Point2<T>(x - v.x, y - v.y);
        }

        Point2<T> operator+(const Vector2<T> &v) const {
            return Point2<T>(x + v.x, y + v.y);
        }

        T x, y;
    };


    using Point2i = Point2<int>;

    template<typename T>
    Point2<T> floor(const Point2<T> &p) {
        return Point2<T>(std::floor(p.x), std::floor(p.y));
    }

    template<typename T>
    Point2<T> ceil(const Point2<T> &p) {
        return Point2<T>(std::ceil(p.x), std::ceil(p.y));
    }

    template<typename T>
    Point2<T> min(const Point2<T> &p1, const Point2<T> &p2) {
        return Point2<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
    }

    template<typename T>
    Point2<T> max(const Point2<T> &p1, const Point2<T> &p2) {
        return Point2<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
    }
    template <typename T, typename U>
    inline Point2<T> operator*(U f, const Point2<T> &p) {
        return p * f;
    }

    template<typename>
    class Vector3;

    template<typename T>
    class Point3 {
    public:
        // Point3 Public Methods
        Point3() { x = y = z = 0; }

        Point3(T x, T y, T z) : x(x), y(y), z(z) {}

        template<typename U>
        explicit Point3(const Point3<U> &p)
                : x((T) p.x), y((T) p.y), z((T) p.z) {

        }

        template<typename U>
        explicit operator Vector3<U>() const {
            return Vector3<U>(x, y, z);
        }

#ifndef NDEBUG

        Point3(const Point3<T> &p) {

            x = p.x;
            y = p.y;
            z = p.z;
        }

        Point3<T> &operator=(const Point3<T> &p) {
            x = p.x;
            y = p.y;
            z = p.z;
            return *this;
        }

#endif  // !NDEBUG

        Point3<T> operator+(const Vector3<T> &v) const {
            return Point3<T>(x + v.x, y + v.y, z + v.z);
        }

        Point3<T> &operator+=(const Vector3<T> &v) {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }

        Vector3<T> operator-(const Point3<T> &p) const {
            return Vector3<T>(x - p.x, y - p.y, z - p.z);
        }

        Point3<T> operator-(const Vector3<T> &v) const {
            return Point3<T>(x - v.x, y - v.y, z - v.z);
        }

        Point3<T> &operator-=(const Vector3<T> &v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }

        Point3<T> &operator+=(const Point3<T> &p) {
            x += p.x;
            y += p.y;
            z += p.z;
            return *this;
        }

        Point3<T> operator+(const Point3<T> &p) const {
            return Point3<T>(x + p.x, y + p.y, z + p.z);
        }

        template<typename U>
        Point3<T> operator*(U f) const {
            return Point3<T>(f * x, f * y, f * z);
        }

        template<typename U>
        Point3<T> &operator*=(U f) {
            x *= f;
            y *= f;
            z *= f;
            return *this;
        }

        template<typename U>
        Point3<T> &operator*=(const Point3<U>& f) {
            x *= f.x;
            y *= f.y;
            z *= f.z;
            return *this;
        }

        template<typename U>
        Point3<T> operator/(U f) const {
            double inv = (double) 1 / f;
            return Point3<T>(inv * x, inv * y, inv * z);
        }

        template<typename U>
        Point3<T> &operator/=(U f) {
            double inv = (double) 1 / f;
            x *= inv;
            y *= inv;
            z *= inv;
            return *this;
        }

        T operator[](int i) const {
            if (i == 0) return x;
            if (i == 1) return y;
            return z;
        }

        T &operator[](int i) {
            if (i == 0) return x;
            if (i == 1) return y;
            return z;
        }

        bool operator==(const Point3<T> &p) const {
            return x == p.x && y == p.y && z == p.z;
        }

        bool operator!=(const Point3<T> &p) const {
            return x != p.x || y != p.y || z != p.z;
        }

        Point3<T> operator-() const { return Point3<T>(-x, -y, -z); }

        T x, y, z;
    };
    template<typename T>
    Point3<T> min(const Point3<T> &p1, const Point3<T> &p2) {
        return Point3<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y),std::min(p1.z,p2.z));
    }

    template<typename T>
    Point3<T> max(const Point3<T> &p1, const Point3<T> &p2) {
        return Point3<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y),std::max(p1.z,p2.z));
    }

    template<typename T,typename U>
    Point3<T> operator*(U u,const Point3<T>& p){
        return p * u;
    }


    template<typename>
    class Normal3;


    template <typename T>
    class Vector3 {
    public:
        // Vector3 Public Methods
        T operator[](int i) const {
            if (i == 0) return x;
            if (i == 1) return y;
            return z;
        }
        T &operator[](int i) {
            if (i == 0) return x;
            if (i == 1) return y;
            return z;
        }
        Vector3() { x = y = z = 0; }
        Vector3(T x, T y, T z) : x(x), y(y), z(z) { }
        explicit Vector3(const Point3<T> &p)
        :x(p.x),y(p.y),z(p.z)
        {}
        explicit Vector3(const Normal3<T> &n);
#ifndef NDEBUG
        // The default versions of these are fine for release builds; for debug
        // we define them so that we can add the Assert checks.
        Vector3(const Vector3<T> &v) {
            x = v.x;
            y = v.y;
            z = v.z;
        }

        Vector3<T> &operator=(const Vector3<T> &v) {
            x = v.x;
            y = v.y;
            z = v.z;
            return *this;
        }
#endif  // !NDEBUG
        Vector3<T> operator+(const Vector3<T> &v) const {
            return Vector3(x + v.x, y + v.y, z + v.z);
        }
        Vector3<T> &operator+=(const Vector3<T> &v) {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }
        Vector3<T> operator-(const Vector3<T> &v) const {
            return Vector3(x - v.x, y - v.y, z - v.z);
        }
        Vector3<T> &operator-=(const Vector3<T> &v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }
        bool operator==(const Vector3<T> &v) const {
            return x == v.x && y == v.y && z == v.z;
        }
        bool operator!=(const Vector3<T> &v) const {
            return x != v.x || y != v.y || z != v.z;
        }

        bool operator!() const noexcept{
           return !x && !y && !z;
        }
        operator bool() const noexcept{
            return x && y && z;
        }

        template <typename U>
        Vector3<T> operator*(U s) const {
            return Vector3<T>(s * x, s * y, s * z);
        }
        template <typename U>
        Vector3<T> &operator*=(U s) {
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }
        template <typename U>
        Vector3<T> operator/(U f) const {
            double inv = (double)1 / f;
            return Vector3<T>(x * inv, y * inv, z * inv);
        }

        template <typename U>
        Vector3<T> &operator/=(U f) {
            double inv = (double)1 / f;
            x *= inv;
            y *= inv;
            z *= inv;
            return *this;
        }
        Vector3<T> operator-() const { return Vector3<T>(-x, -y, -z); }
        double length_squared() const { return x * x + y * y + z * z; }
        double length() const { return std::sqrt(length_squared()); }

        // Vector3 Public Data
        T x, y, z;
    };

    template <typename T>
    class Normal3 {
    public:
        // Normal3 Public Methods
        Normal3() { x = y = z = 0; }
        Normal3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) { }
        Normal3<T> operator-() const { return Normal3(-x, -y, -z); }
        Normal3<T> operator+(const Normal3<T> &n) const {
            return Normal3<T>(x + n.x, y + n.y, z + n.z);
        }

        Normal3<T> &operator+=(const Normal3<T> &n) {
            x += n.x;
            y += n.y;
            z += n.z;
            return *this;
        }
        Normal3<T> operator-(const Normal3<T> &n) const {
            return Normal3<T>(x - n.x, y - n.y, z - n.z);
        }

        Normal3<T> &operator-=(const Normal3<T> &n) {
            x -= n.x;
            y -= n.y;
            z -= n.z;
            return *this;
        }

        template <typename U>
        Normal3<T> operator*(U f) const {
            return Normal3<T>(f * x, f * y, f * z);
        }

        template <typename U>
        Normal3<T> &operator*=(U f) {
            x *= f;
            y *= f;
            z *= f;
            return *this;
        }
        template <typename U>
        Normal3<T> operator/(U f) const {
            double inv = (double)1 / f;
            return Normal3<T>(x * inv, y * inv, z * inv);
        }

        template <typename U>
        Normal3<T> &operator/=(U f) {
            double inv = (double)1 / f;
            x *= inv;
            y *= inv;
            z *= inv;
            return *this;
        }
        double length_squared() const { return x * x + y * y + z * z; }
        double length() const { return std::sqrt(length_squared()); }

#ifndef NDEBUG
        Normal3<T>(const Normal3<T> &n) {
            x = n.x;
            y = n.y;
            z = n.z;
        }

        Normal3<T> &operator=(const Normal3<T> &n) {
            x = n.x;
            y = n.y;
            z = n.z;
            return *this;
        }
#endif  // !NDEBUG
        explicit Normal3<T>(const Vector3<T> &v) : x(v.x), y(v.y), z(v.z) {
        }
        bool operator==(const Normal3<T> &n) const {
            return x == n.x && y == n.y && z == n.z;
        }
        bool operator!=(const Normal3<T> &n) const {
            return x != n.x || y != n.y || z != n.z;
        }

        T operator[](int i) const {
            if (i == 0) return x;
            if (i == 1) return y;
            return z;
        }

        T &operator[](int i) {
            if (i == 0) return x;
            if (i == 1) return y;
            return z;
        }

        Normal3& normalize() {
            auto len = length();
            x /= len;
            y /= len;
            z /= len;
            return *this;
        }

        T x, y, z;
    };
    using Normal3f = Normal3<real>;

    template<typename T>
    Vector3<T> operator*(T t,const Vector3<T>& v){
        return v * t;
    }

    template<typename T>
    Vector3<T>::Vector3(const Normal3<T> &n)
    :x(n.x),y(n.y),z(n.z)
    {

    }

    template <typename T>
    inline Vector3<T> normalize(const Vector3<T> &v) {
        return v / v.length();
    }

    template <typename T>
    inline Normal3<T> normalize(const Normal3<T> &v) {
        return v / v.length();
    }

    template <typename T>
    inline Vector3<T> cross(const Vector3<T> &v1, const Vector3<T> &v2) {
        double v1x = v1.x, v1y = v1.y, v1z = v1.z;
        double v2x = v2.x, v2y = v2.y, v2z = v2.z;
        return Vector3<T>((v1y * v2z) - (v1z * v2y), (v1z * v2x) - (v1x * v2z),
                          (v1x * v2y) - (v1y * v2x));
    }

    template<typename T>
    class Bounds2 {
    public:
        Bounds2(const Point2<T> &low, const Point2<T> &high)
                : low(low), high(high) {}

        template<typename U>
        explicit operator Bounds2<U>() const {
            return Bounds2<U>((Point2<U>) low, (Point2<U>) high);
        }

        bool operator==(const Bounds2<T> &b) const {
            return b.low == low && b.high == high;
        }

        bool operator!=(const Bounds2<T> &b) const {
            return b.low != low || b.high != high;
        }

        T area() const {
            Point2<T> d = high - low;
            return d.x * d.y;
        }

        Point2<T> low, high;
    };

    using Bounds2i = Bounds2<int>;

    template<typename T>
    Bounds2<T> intersect(const Bounds2<T> &b1, const Bounds2<T> &b2) {

        return Bounds2<T>(max(b1.low, b2.low), min(b1.high, b2.high));
    }
    using Point2f = Point2<real>;
    using Point3f = Point3<real>;
    using Point3b = Point3<uint8_t>;
    using Vector3i = Vector3<int>;
    using Vector2i = Vector2<int>;
    using Vector3f = Vector3<real>;
    using Vector2f = Vector2<real>;
    using Bounds2f = Bounds2<real>;
    using Color3b = Point3b;
    using Color3f = Point3f;

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


    template <typename T>
    class Bounds3 {
    public:
        Bounds3() {
            T minNum = std::numeric_limits<T>::lowest();
            T maxNum = std::numeric_limits<T>::max();
            low = Point3<T>(maxNum, maxNum, maxNum);
            high = Point3<T>(minNum, minNum, minNum);
        }
        explicit Bounds3(const Point3<T> &p) : low(p), high(p) {}
        Bounds3(const Point3<T> &p1, const Point3<T> &p2)
                : low(std::min(p1.x, p2.x), std::min(p1.y, p2.y),
                       std::min(p1.z, p2.z)),
                  high(std::max(p1.x, p2.x), std::max(p1.y, p2.y),
                       std::max(p1.z, p2.z)) {}
        const Point3<T> &operator[](int i) const{
            return i == 0 ? low : high;
        }
        Point3<T> &operator[](int i){
            return i == 0 ? low : high;
        }
        bool operator==(const Bounds3<T> &b) const {
            return b.pMin == low && b.pMax == high;
        }
        bool operator!=(const Bounds3<T> &b) const {
            return b.pMin != low || b.pMax != high;
        }
        Point3<T> corner(int corner) const {
            return Point3<T>((*this)[(corner & 1)].x,
                             (*this)[(corner & 2) ? 1 : 0].y,
                             (*this)[(corner & 4) ? 1 : 0].z);
        }
        Vector3<T> diagonal() const { return high - low; }
        T surface_area() const {
            Vector3<T> d = diagonal();
            return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
        }
        T volume() const {
            Vector3<T> d = diagonal();
            return d.x * d.y * d.z;
        }
        int maximum_extent() const {
            Vector3<T> d = diagonal();
            if (d.x > d.y && d.x > d.z)
                return 0;
            else if (d.y > d.z)
                return 1;
            else
                return 2;
        }

        Vector3<T> offset(const Point3<T> &p) const {
            Vector3<T> o = p - low;
            if (high.x > low.x) o.x /= high.x - low.x;
            if (high.y > low.y) o.y /= high.y - low.y;
            if (high.z > low.z) o.z /= high.z - low.z;
            return o;
        }
        void bounding_sphere(Point3<T> *center, real *radius) const {
            *center = (low + high) / 2;
            *radius = Inside(*center, *this) ? Distance(*center, high) : 0;
        }
        template <typename U>
        explicit operator Bounds3<U>() const {
            return Bounds3<U>((Point3<U>)low, (Point3<U>)high);
        }
        bool intersect_p(const Ray &ray, real *hitt0 = nullptr,
                        real *hitt1 = nullptr) const;
        bool intersect_p(const Ray &ray, const Vector3f &invDir,
                               const int dirIsNeg[3]) const;

        Point3<T> low, high;
    };

    using Bounds3f = Bounds3<real>;
    using Bounds3i = Bounds3<int>;


    template <typename T>
    Bounds3<T> Union(const Bounds3<T> &b, const Point3<T> &p) {
        Bounds3<T> ret;
        ret.low = min(b.low, p);
        ret.high = max(b.high, p);
        return ret;
    }

    template <typename T>
    Bounds3<T> Union(const Bounds3<T> &b1, const Bounds3<T> &b2) {
        Bounds3<T> ret;
        ret.low = min(b1.low, b2.low);
        ret.high = max(b1.high, b2.high);
        return ret;
    }


    class Ray{
    public:
        Ray(const Point3f& o, const Vector3f& d,real t_min = 0,real t_max = REAL_MAX)
                :o(o),d(normalize(d)),t(0),t_min(t_min),t_max(t_max)
        {}
        Ray()
                :Ray(Point3f(),Vector3f(1,0,0))
        {}

        Point3f operator()(real time) const{
            return o + d * time;
        }


        Point3f o;
        Vector3f d;
        real t;
        mutable real t_min;
        mutable real t_max;
    };

    template <typename T>
    inline bool Bounds3<T>::intersect_p(const Ray &ray, real *hitt0,
                                       real *hitt1) const {
        real t0 = 0, t1 = ray.t_max;
        for (int i = 0; i < 3; ++i) {
            // Update interval for _i_th bounding box slab
            real invRayDir = 1 / ray.d[i];
            real tNear = (low[i] - ray.o[i]) * invRayDir;
            real tFar = (high[i] - ray.o[i]) * invRayDir;

            // Update parametric interval from slab intersection $t$ values
            if (tNear > tFar) std::swap(tNear, tFar);

            // Update _tFar_ to ensure robust ray--bounds intersection
            tFar *= 1 + 0.0000001;
            t0 = tNear > t0 ? tNear : t0;
            t1 = tFar < t1 ? tFar : t1;
            if (t0 > t1) return false;
        }
        if (hitt0) *hitt0 = t0;
        if (hitt1) *hitt1 = t1;
        return true;
    }

    template <typename T>
    inline bool Bounds3<T>::intersect_p(const Ray &ray, const Vector3f &invDir,
                                       const int dirIsNeg[3]) const {
        const Bounds3f &bounds = *this;
        // Check for ray intersection against $x$ and $y$ slabs
        real tMin = (bounds[dirIsNeg[0]].x - ray.o.x) * invDir.x;
        real tMax = (bounds[1 - dirIsNeg[0]].x - ray.o.x) * invDir.x;
        real tyMin = (bounds[dirIsNeg[1]].y - ray.o.y) * invDir.y;
        real tyMax = (bounds[1 - dirIsNeg[1]].y - ray.o.y) * invDir.y;

        // Update _tMax_ and _tyMax_ to ensure robust bounds intersection
        tMax *= 1 + 0.0000001;
        tyMax *= 1 + 0.0000001;
        if (tMin > tyMax || tyMin > tMax) return false;
        if (tyMin > tMin) tMin = tyMin;
        if (tyMax < tMax) tMax = tyMax;

        // Check for ray intersection against $z$ slab
        real tzMin = (bounds[dirIsNeg[2]].z - ray.o.z) * invDir.z;
        real tzMax = (bounds[1 - dirIsNeg[2]].z - ray.o.z) * invDir.z;

        // Update _tzMax_ to ensure robust bounds intersection
        tzMax *= 1 + 0.0000001;
        if (tMin > tzMax || tzMin > tMax) return false;
        if (tzMin > tMin) tMin = tzMin;
        if (tzMax < tMax) tMax = tzMax;
        return (tMin < ray.t_max) && (tMax > 0);
    }

    template <typename T>
    inline T dot(const Normal3<T> &n1, const Vector3<T> &v2) {
        return n1.x * v2.x + n1.y * v2.y + n1.z * v2.z;
    }

    template <typename T>
    inline T dot(const Vector3<T> &n1, const Vector3<T> &v2) {
        return n1.x * v2.x + n1.y * v2.y + n1.z * v2.z;
    }

    template <typename T>
    inline T dot(const Vector3<T> &v1, const Normal3<T> &n2) {
        return v1.x * n2.x + v1.y * n2.y + v1.z * n2.z;
    }

    template <typename T>
    inline T dot(const Normal3<T> &n1, const Normal3<T> &n2) {
        return n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
    }


    template<typename T>
    inline void coordinate(const Vector3<T>& v1,Vector3<T>& v2,Vector3<T>& v3){
        if(std::abs(v1.x) > std::abs(v1.y))
            v2 = Vector3<T>(-v1.z,0,v1.x) / std::sqrt(v1.x*v1.x+v1.z*v1.z);
        else
            v2 = Vector3<T>(0,v1.z,-v1.y) / std::sqrt(v1.y*v1.y+v1.z*v1.z);
        v3 = cross(v1,v2);
    }

    template <typename T>
    inline T abs_dot(const Normal3<T> &n1, const Vector3<T> &v2) {
        return std::abs(n1.x * v2.x + n1.y * v2.y + n1.z * v2.z);
    }

    template <typename T>
    inline T abs_dot(const Vector3<T> &v1, const Normal3<T> &n2) {
        return std::abs(v1.x * n2.x + v1.y * n2.y + v1.z * n2.z);
    }

    template <typename T>
    inline T abs_dot(const Normal3<T> &n1, const Normal3<T> &n2) {
        return std::abs(n1.x * n2.x + n1.y * n2.y + n1.z * n2.z);
    }
}

namespace std{
    template<>
    struct hash<tracer::Point3f>{
        size_t operator()(const tracer::Point3f& p) const{
            return ::hash(p.x,p.y,p.z);
        }
    };

    template<>
    struct hash<tracer::Point2f>{
        size_t operator()(const tracer::Point2f& p) const{
            return ::hash(p.x,p.y);
        }
    };

    template<>
    struct hash<tracer::Normal3f>{
        size_t operator()(const tracer::Normal3f& p) const{
            return ::hash(p.x,p.y,p.z);
        }
    };
}

#endif //TRACER_GEOMETRY_HPP
