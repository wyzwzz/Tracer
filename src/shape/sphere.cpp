
#include "transformed_shape.hpp"
#include "core/sampling.hpp"
TRACER_BEGIN

namespace sphere{
    inline std::tuple<real, real, real> get_abc(real radius_square, const Ray &r)
    {
        const real A = r.d.length_squared();
        const real B = 2 * dot((Vector3f)r.d, (Vector3f)r.o);
        const real C = Vector3f(r.o).length_squared() - radius_square;
        return { A, B, C };
    }

    bool intersect(const Ray& r,real radius) noexcept{
        const auto [A, B, C] = get_abc(radius * radius, r);
        real delta = B * B - 4 * A * C;
        if(delta < 0)
            return false;
        delta = std::sqrt(delta);

        const real inv2A = real(0.5) / A;
        const real t0 = (-B - delta) * inv2A;
        const real t1 = (-B + delta) * inv2A;

        return r.between(t0) || r.between(t1);
    }
    bool intersect_p(const Ray& r,real* t,real radius) noexcept{
        auto [A, B, C] = sphere::get_abc(radius * radius, r);
        real delta = B * B - 4 * A * C;
        if(delta < 0)
            return false;
        delta = std::sqrt(delta);

        real inv2A = real(0.5) / A;
        real t0 = (-B - delta) * inv2A;
        real t1 = (-B + delta) * inv2A;

        if(r.t_max < t0 || t1 < r.t_min)
            return false;
        bool not_t0 = t0 < r.t_min;
        if(not_t0 && t1 > r.t_max)
            return false;

        *t = not_t0 ? t1 : t0;
        return true;
    }
    void local_geometry_uv_and_coord(const Point3f& local_pos,Point2f& uv,Coord& local_coord,real radius) noexcept{
        real phi = (!local_pos.x && !local_pos.y) ?
                   real(0) : std::atan2(local_pos.y, local_pos.x);
        if(phi < 0)
            phi += 2 * PI_r;
        const real sin_theta = std::clamp<real>(local_pos.z / radius, -1, 1);
        const real theta = std::asin(sin_theta);

        uv = Point2f(real(0.5) * invPI_r * phi, invPI_r * theta + real(0.5));

        Vector3f ex, ey, ez = Vector3f(local_pos).normalize();
        if(std::abs(ez.z - 1) < eps)
        {
            ex = Vector3f(1, 0, 0);
            ey = Vector3f(0, 1, 0);
            ez = Vector3f(0, 0, 1);
        }
        else if(std::abs(ez.z + 1) < eps)
        {
            ex = Vector3f(0, 1, 0);
            ey = Vector3f(1, 0, 0);
            ez = Vector3f(0, 0, -1);
        }
        else
        {
            ex = cross({ 0, 0, 1 }, ez);
            ey = cross(ez, ex);
        }

        local_coord = Coord(ex, ey, ez);
    }


}

class Sphere:public TransformedShape{
public:
    Sphere(real radius,const Transform& local_to_world)
    : TransformedShape(local_to_world),radius(radius)
    {
        if(radius <= 0){
            throw std::runtime_error("invalid sphere radius");
        }
        world_radius = local_to_world_scale_ratio * radius;
    }

    bool intersect(const Ray& ray) const noexcept override{
        return sphere::intersect(to_local(ray),radius);
    }

    bool intersect_p(const Ray& ray,real* hit_t,SurfaceIntersection* isect) const noexcept override{
        const Ray local_ray = to_local(ray);
        real t;
        if(!sphere::intersect_p(local_ray,&t,radius))
            return false;

        const Point3f pos = local_ray(t);

        Point2f geometry_uv;
        Coord geometry_coord;
        sphere::local_geometry_uv_and_coord(pos,geometry_uv,geometry_coord,radius);

        isect->pos = pos;
        isect->geometry_coord = geometry_coord;
        isect->uv = geometry_uv;
        isect->shading_coord = geometry_coord;
        isect->wo = -local_ray.d;

        to_world(*isect);

        return true;
    }

    Bounds3f world_bound() const noexcept override{
        const Point3f world_origin = local_to_world(Point3f(0));
        return {
                world_origin - Vector3f(world_radius + eps),
                world_origin + Vector3f(world_radius + eps)
        };
    }

    real surface_area() const noexcept override{
        return 4 * PI_r * world_radius * world_radius;
    }

    SurfacePoint sample(real* pdf,const Sample2& sample) const noexcept override{
        const auto [unit_pos, unit_pdf] = uniform_on_sphere(sample.u,sample.v);

        *pdf = unit_pdf / (world_radius * world_radius);

        const Point3f pos = radius * unit_pos;

        Point2f geometry_uv;
        Coord geometry_coord;
        sphere::local_geometry_uv_and_coord(
                pos, geometry_uv, geometry_coord, radius);

        SurfacePoint spt;
        spt.pos            = pos;
        spt.geometry_coord = geometry_coord;
        spt.uv             = geometry_uv;
        spt.shading_coord     = geometry_coord;
        to_world(spt);
        return spt;
    }

    SurfacePoint sample(const Point3f& ref,real* pdf,const Sample2& sample_) const noexcept override{
        const Point3f local_ref = world_to_local(ref);
        const real d = Vector3f(local_ref).length();
        if(d <= radius)
            return sample(pdf, sample_);

        const real cos_theta = (std::min)(radius / d, real(1));
        const auto [_dir, l_pdf] = uniform_on_cone(cos_theta, sample_.u, sample_.v);
        auto dir = Vector3f(_dir);
        const Point3f pos = Point3f(radius * Coord::create_from_z((Vector3f)local_ref).local_to_global(dir).normalize());

        Point2f geometry_uv;
        Coord geometry_coord;
        sphere::local_geometry_uv_and_coord(
                pos, geometry_uv, geometry_coord, radius);

        SurfacePoint spt;
        spt.pos            = pos;
        spt.geometry_coord = geometry_coord;
        spt.uv             = geometry_uv;
        spt.shading_coord     = geometry_coord;
        to_world(spt);

        const real world_radius_square = world_radius * world_radius;
        *pdf = l_pdf / world_radius_square;

        return spt;
    }

    real pdf(const Point3f& pos) const noexcept override{
        return 1 / surface_area();
    }

    real pdf(const Point3f& ref,const Point3f& pos) const noexcept override{
        const Point3f local_ref = world_to_local(ref);
        const real d = Vector3f(local_ref).length();
        if(d <= radius)
            return pdf(pos);

        const real cos_theta = (std::min)(radius/ d, real(1));
        const real world_radius_square = world_radius * world_radius;

        return uniform_on_cone_pdf(
                cos_theta) / world_radius_square;
    }

private:
    real radius;
    real world_radius;
};

RC<Shape> create_sphere(real radius,const Transform& local_to_world){
    return newRC<Sphere>(radius,local_to_world);
}

TRACER_END