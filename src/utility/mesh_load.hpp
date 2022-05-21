//
// Created by wyz on 2022/5/20.
//

#ifndef TRACER_MESH_LOAD_HPP
#define TRACER_MESH_LOAD_HPP

#include "geometry.hpp"
#include "core/texture.hpp"
#include "utility/image.hpp"
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

        //todo change to Spectrum
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

        std::string map_ka;
        std::string map_kd;
        std::string map_ks;
        std::string map_ns;
        std::string map_bump;
        std::string disp;
        std::string map_d;//displayment
        std::string refl;

        //PBR extension
        real roughness;
        real metallic;
        real sheen;
        real clearcoat_thickness;
        real clearcoat_roughness;

        std::string map_pr;//roughness
        std::string map_pm;//metallic
        std::string map_ps;//sheen
        std::string map_ke;
        std::string norm;
    };

    struct mesh_t
    {
        std::string name;
        std::vector<vertex_t> vertices;
        std::vector<int> indices;
        //一个mesh可以对应多个material
        //一个三角形或者shape可以单独对应一个material
        std::vector<int> materials;
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
