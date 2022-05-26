//
// Created by wyz on 2022/5/19.
//
#include "core/shape.hpp"
#include "utility/geometry.hpp"
#include "utility/transform.hpp"
#include "utility/mesh_load.hpp"
#include "core/intersection.hpp"
#include "core/sampling.hpp"
#include "utility/logger.hpp"
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
            Point3f A = mesh->p[vertex[0]];
            Point3f B = mesh->p[vertex[1]];
            Point3f C = mesh->p[vertex[2]];
            return 0.5 * cross(B-A,C-A).length();
        }

        SurfacePoint sample(real* pdf,const Sample2& sam) const noexcept{
            auto b = UniformSampleTriangle(sam);
            const auto& A = mesh->p[vertex[0]];
            const auto& B = mesh->p[vertex[1]];
            const auto& C = mesh->p[vertex[2]];
            const auto& uvA = mesh->uv[vertex[0]];
            const auto& uvB = mesh->uv[vertex[1]];
            const auto& uvC = mesh->uv[vertex[2]];
            SurfacePoint sp;
            sp.pos = b.x * A + b.y * B + (1 - b.x - b.y) * C;
            sp.n = Normal3f(normalize(cross(B-A,C-A)));
            sp.uv = b.x * uvA + b.y * uvB + (1 - b.x - b.y) * uvC;
            *pdf = 1 / surface_area();
            return sp;
        }

        SurfacePoint sample(const SurfacePoint& ref,real* pdf,const Sample2& sam) const noexcept{
            return sample(pdf,sam);
        }

        real pdf(const SurfacePoint& p) const noexcept{
            NOT_IMPL
            return 0;
        }

        real pdf(const SurfacePoint&ref, const Vector3f& wi) const noexcept{
            NOT_IMPL
            return 0;
        }

        real pdf(const Point3f& ref,const Point3f& pos) const noexcept override{
                return 1 / surface_area();
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
//        ray.t_max = t;

        const Point2f uvA = mesh->uv[vertex[0]];
        const Point2f uvB = mesh->uv[vertex[1]];
        const Point2f uvC = mesh->uv[vertex[2]];

        const Vector3f nA = (Vector3f)mesh->n[vertex[0]].normalize();
        const Vector3f nB = (Vector3f)mesh->n[vertex[1]].normalize();
        const Vector3f nC = (Vector3f)mesh->n[vertex[2]].normalize();
        //todo
        isect->n = (Normal3f)normalize(cross(AB,AC));
        isect->uv = uvA + alpha * (uvB - uvA) + beta * (uvC - uvA);
        isect->wo = -ray.d;

        isect->pos = ray(t);


        auto compute_ss_ts = [&](const Vector3f& AB,const Vector3f& AC,
                const Vector2f& ab,const Vector2f& ac,const Vector3f& n,
                Vector3f& ss,Vector3f& ts){
            const real m00 = ab.x, m01 = ab.y;
            const real m10 = ac.x, m11 = ac.y;
            const real det = m00 * m11 - m01 * m10;
            if(det){
                const real inv_det = 1 / det;
                ss  = m11 * inv_det * AB - m01 * inv_det * AC;
                ts = m10 * inv_det * AB - m00 * inv_det * AC;
            }
            else{
                coordinate(n,ss,ts);
            }
        };
        //compute dpdu dpdv
        compute_ss_ts(AB,AC,Vector2f(uvB - uvA),Vector2f(uvC-uvA),(Vector3f)isect->n,isect->dpdu,isect->dpdv);

        //compute dndu dndv
        compute_ss_ts(nB-nA,nC-nA,Vector2f(uvB-uvA),Vector2f(uvC-uvA),(Vector3f)isect->n,isect->dndu,isect->dndv);
        if(isect->dndu.length_squared() < eps || isect->dndv.length_squared() < eps){
            coordinate((Vector3f)isect->n,isect->dndu,isect->dndv);
        }

        auto ns = Normal3f(nA + alpha * (nB - nA) + beta * (nC - nA)).normalize();
        auto ss = normalize(isect->dpdu);
        auto ts = cross(ss,(Vector3f)ns);//todo ???
        if(ts.length_squared() > 0){
            ts = normalize(ts);
            ss = cross(ts,(Vector3f)ns);
        }
        else
            coordinate((Vector3f)ns,ss,ts);

        compute_ss_ts(nB-nA,nC-nA,Vector2f(uvB-uvA),Vector2f(uvC-uvA),(Vector3f)ns,isect->shading.dndu,isect->shading.dndv);

        isect->shading.dpdu = ss;
        isect->shading.dpdv = ts;
        isect->shading.n = (Normal3f)normalize(cross(ss,ts));

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