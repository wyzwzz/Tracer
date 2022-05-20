//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_MESH_LOAD_HPP
#define TRACER_MESH_LOAD_HPP

#include "geometry.hpp"
#include "core/texture.hpp"
TRACER_BEGIN

    struct material_t;

    struct vertex_t{
        Point3f pos;
        Normal3f n;
        Point2f uv;
        bool operator==(const vertex_t &other) const
        {
            return pos == other.pos && n == other.n && uv == other.uv;
        }


    };

    struct material_t{
        std::string name;

        real ambient[3];
        real diffuse[3];
        real specular[3];
        real transmittance[3];
        real emission[3];
        real shininess;
        real ior;//index of refraction
        real dissolve;//1 == opaque; 0 == fully transparent

        //0 Color on and Ambient off
        //1 Color on and Ambient on
        //2 Highlight on
        //3 Reflection on and Ray trace on
        //4 Transparency: Glass on
        //Reflection: Ray trace on
        //5 Reflection: Fresnel on and Ray trace on
        //6 Transparency: Refraction on
        //Reflection: Fresnel off and Ray trace on
        //7 Transparency: Refraction on
        //Reflection: Fresnel on and Ray trace on
        //8 Reflection on and Ray trace off
        //9 Transparency: Glass on
        //Reflection: Ray trace off
        //10 Casts shadows onto invisible surfaces
        int illum;

        RC<Texture2D> map_ka;
        RC<Texture2D> map_kd;
        RC<Texture2D> map_ks;
        RC<Texture2D> map_ns;
        RC<Texture2D> map_bump;
        RC<Texture2D> disp;
        RC<Texture2D> map_d;
        RC<Texture2D> refl;

        //PBR extension
        RC<Texture2D> map_pr;//roughness
        RC<Texture2D> map_pm;//metallic
        RC<Texture2D> map_ps;//sheen
        RC<Texture2D> map_ke;
        RC<Texture2D> norm;
    };

    struct mesh_t
    {
        std::string name;
        std::vector<vertex_t> vertices;
        std::vector<int> indices;
        //一个mesh可以对应多个material
        //一个三角形或者shape可以单独对应一个material
        std::vector<material_t*> materials;
    };

    struct model_t{
        std::string name;
        std::vector<mesh_t> mesh;
        std::vector<material_t> material;

    };

    model_t load_model_from_file(const std::string& name);

TRACER_END

namespace std
{
    template <> struct hash<tracer::vertex_t>
    {
        size_t operator()(const tracer::vertex_t &vertex) const
        {
            return ::hash(vertex.pos,vertex.n,vertex.uv);
        }
    };
} // namespace std

#endif //TRACER_MESH_LOAD_HPP
