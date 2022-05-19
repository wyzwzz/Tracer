//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_GEOMETRY_HPP
#define TRACER_GEOMETRY_HPP


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
        Point2(T xx, T yy) : x(xx), y(yy) {}

        template<typename U>
        explicit Point2(const Point2<U> &p) {
            x = (T) p.x;
            y = (T) p.y;
        }

        bool operator==(const Point2<T> &p) const { return x == p.x && y == p.y; }

        bool operator!=(const Point2<T> &p) const { return x != p.x || y != p.y; }

        Point2 operator+(const Point2 &p) const {
            return Point2(x + p.x, y + p.y);
        }

        Point2 operator+(T t) const {
            return Point2(x + t, y + t);
        }

        Point2 operator-(const Point2 &p) const {
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

        T x, y;
    };


    using Point2i = Point2<int>;

    template<typename T>
    Point2<T> Floor(const Point2<T> &p) {
        return Point2<T>(std::floor(p.x), std::floor(p.y));
    }

    template<typename T>
    Point2<T> Ceil(const Point2<T> &p) {
        return Point2<T>(std::ceil(p.x), std::ceil(p.y));
    }

    template<typename T>
    Point2<T> Min(const Point2<T> &p1, const Point2<T> &p2) {
        return Point2<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
    }

    template<typename T>
    Point2<T> Max(const Point2<T> &p1, const Point2<T> &p2) {
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
        explicit Vector3(const Normal3<T> &n);

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

        T x, y, z;
    };


    template <typename T>
    inline Vector3<T> normalize(const Vector3<T> &v) {
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
    Bounds2<T> Intersect(const Bounds2<T> &b1, const Bounds2<T> &b2) {

        return Bounds2<T>(Max(b1.low, b2.low), Min(b1.high, b2.high));
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




}

#endif //TRACER_GEOMETRY_HPP
