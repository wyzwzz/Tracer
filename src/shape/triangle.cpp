//
// Created by wyz on 2022/5/19.
//
#include "core/shape.hpp"
#include "utility/geometry.hpp"
#include <vector>
TRACER_BEGIN


    struct TriangleMesh{
        const int triangles_count;
        const int vertices_count;
        Box<Point3f[]> p;
        Box<Normal3f[]> n;
        Box<Point2f[]> uv;
    };

    class Triangle:public Shape{
    public:


    private:
        RC<TriangleMesh> mesh;
    };

    std::vector<RC<Triangle>> create_triangle_mesh(){

    }

TRACER_END