//
// Created by wyz on 2022/5/19.
//
#include "core/shape.hpp"
#include "utility/geometry.hpp"
#include "utility/transform.hpp"
#include "utility/mesh_load.hpp"
#include "core/intersection.hpp"
#include <vector>
TRACER_BEGIN

    //一个obj文件可能有多个mesh 每个mesh应该分别对应一个TriangleMesh 因为各自有其Transform
    struct TriangleMesh{
        TriangleMesh(const Transform& t,int tri_count,int v_count)
        :local_to_world(t),triangles_count(tri_count),vertices_count(v_count),
        indices(tri_count * 3),
        p(newBox<Point3f[]>(v_count)),
        n(newBox<Normal3f[]>(v_count)),
        uv(newBox<Point2f[]>(v_count))
        {
            assert(triangles_count && vertices_count);
        }
        const int triangles_count;
        const int vertices_count;
        std::vector<int> indices;
        Box<Point3f[]> p;
        Box<Normal3f[]> n;
        Box<Point2f[]> uv;
        Transform local_to_world;

    };

    class Triangle:public Shape{
    public:
        Triangle(const RC<TriangleMesh>& m,int triangle_index)
        :mesh(m)
        {
            vertex = &m->indices[triangle_index * 3];
        }

        bool intersect(const Ray& ray) const noexcept ;

        bool intersect_p(const Ray& ray,real* hit_t,SurfaceIntersection* isect) const noexcept ;

        Bounds3f world_bound() const noexcept{
            const Point3f& A = mesh->p[vertex[0]];
            const Point3f& B = mesh->p[vertex[1]];
            const Point3f& C = mesh->p[vertex[2]];
            return Union(Bounds3f(A,B),C);
        }

        real surface_area() const noexcept {
            return 0;
        }

        SurfacePoint sample(real* pdf,const Sample2& sample) const noexcept{
            return {};
        }

        SurfacePoint sample(const SurfacePoint& ref,real* pdf,const Sample2& sample) const noexcept{
            return {};
        }

        real pdf(const SurfacePoint& p) const noexcept{
            return 0;
        }

        real pdf(const SurfacePoint&ref, const Vector3f& wi) const noexcept{
            return 0;
        }

    private:
        RC<TriangleMesh> mesh;
        const int* vertex = nullptr;

    };

    bool Triangle::intersect(const Ray& ray) const noexcept {
        const Point3f A = mesh->p[vertex[0]];
        const Point3f B = mesh->p[vertex[1]];
        const Point3f C = mesh->p[vertex[2]];

        const Vector3f AB = B - A;
        const Vector3f AC = C - A;

        Vector3f s1 = cross(ray.d, AC);
        real div = dot(s1,AB);
        if(!div) return false;

        real inv_div = 1 / div;

        const Vector3f AO = ray.o - A;
        real alpha = dot(AO,s1) * inv_div;
        if(alpha < 0) return false;

        Vector3f s2 = cross(AO,AB);
        real beta = dot(ray.d,s2) * inv_div;
        if(beta < 0 || alpha + beta > 1)
            return false;
        real t = dot(AC,s2) * inv_div;

        if(t < ray.t_min || t > ray.t_max) return false;
        return true;
    }

    bool Triangle::intersect_p(const Ray &ray, real *hit_t, SurfaceIntersection *isect) const noexcept {

        //需要计算相交点的局部坐标系
        //通过三角形的三个点的坐标和纹理坐标计算出
        //计算出的局部坐标系的z应该是面的法向量

        const Point3f A = mesh->p[vertex[0]];
        const Point3f B = mesh->p[vertex[1]];
        const Point3f C = mesh->p[vertex[2]];

        const Vector3f AB = B - A;
        const Vector3f AC = C - A;

        Vector3f s1 = cross(ray.d, AC);
        real div = dot(s1,AB);
        if(!div) return false;

        real inv_div = 1 / div;

        const Vector3f AO = ray.o - A;
        real alpha = dot(AO,s1) * inv_div;
        if(alpha < 0) return false;

        Vector3f s2 = cross(AO,AB);
        real beta = dot(ray.d,s2) * inv_div;
        if(beta < 0 || alpha + beta > 1)
            return false;
        real t = dot(AC,s2) * inv_div;

        if(t < ray.t_min || t > ray.t_max) return false;
        *hit_t = t;
        ray.t_max = t;

        const Point2f uvA = mesh->uv[vertex[0]];
        const Point2f uvB = mesh->uv[vertex[1]];
        const Point2f uvC = mesh->uv[vertex[2]];

        //todo

        isect->uv = uvA + alpha * (uvB - uvA) + beta * (uvC - uvA);

        isect->pos = ray(t);


        return true;
    }

//    RC<Shape> create_triangle(){
//
//    }

    std::vector<RC<Shape>> create_triangle_mesh(const mesh_t& mesh){
        std::vector<RC<Shape>> triangles;
        const auto triangles_count = mesh.indices.size() / 3;
        const auto vertices_count = mesh.vertices.size();
        assert(mesh.indices.size() % 3 == 0);
        Transform t;
        auto triangle_mesh = newRC<TriangleMesh>(t,triangles_count,vertices_count);
        triangle_mesh->indices = mesh.indices;
        for(size_t i = 0; i < vertices_count; ++i){
            const auto& vertex = mesh.vertices[i];
            triangle_mesh->p[i] = t(vertex.pos);
            triangle_mesh->n[i] = t(vertex.n);
            triangle_mesh->uv[i] = vertex.uv;
        }
        for(size_t i = 0; i < triangles_count; ++i){
            triangles.emplace_back(newRC<Triangle>(triangle_mesh,i));
        }


        return triangles;
    }

TRACER_END