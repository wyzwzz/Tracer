//
// Created by wyz on 2022/5/20.
//
#include "mesh_load.hpp"
#include "utility/logger.hpp"
#include <tiny_obj_loader.h>
TRACER_BEGIN

    std::string extract_name_from_path(const std::string& path){
         return path.empty() ? path : path.substr(std::min(path.find_last_of('\\'),path.find_last_of('/')));
    }

    model_t load_model_from_file(const std::string& path){
        model_t model;
        model.name = extract_name_from_path(path);
        tinyobj::ObjReader reader;
        if(!reader.ParseFromFile(path)){
            LOG_CRITICAL("load model from obj file error: {}",reader.Error());
            throw std::runtime_error("failed to load obj file");
        }
        if(!reader.Warning().empty()){
            LOG_ERROR("load model form obj warning: {}",reader.Warning());
        }

        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();
        const auto& materials = reader.GetMaterials();
        for(const auto& m:materials){
            model.material.emplace_back();
            auto& material = model.material.back();
            std::copy(m.ambient,m.ambient+3,material.ambient);//copy ambient
            std::copy(m.diffuse,m.diffuse+3,material.diffuse);
            std::copy(m.specular,m.specular+3,material.specular);
            std::copy(m.transmittance,m.transmittance+3,material.transmittance);
            std::copy(m.emission,m.emission+3,material.emission);
            material.shininess = m.shininess;
            material.ior = m.ior;
            material.dissolve = m.dissolve;
            material.illum = m.illum;

            material.map_ka = m.ambient_texname;
            material.map_kd = m.diffuse_texname;
            material.map_ks = m.specular_texname;
            material.map_ns = m.specular_highlight_texname;
            material.map_bump = m.bump_texname;
            material.disp = m.displacement_texname;
            material.map_d = m.displacement_texname;
            material.refl = m.reflection_texname;

            material.roughness = m.roughness;
            material.metallic = m.metallic;
            material.sheen = m.sheen;
            material.clearcoat_thickness = m.clearcoat_thickness;
            material.clearcoat_roughness = m.clearcoat_roughness;

            material.map_pr = m.roughness_texname;
            material.map_pm = m.metallic_texname;
            material.map_ps = m.sheen_texname;
            material.map_ke = m.emissive_texname;
            material.norm = m.normal_texname;
        }
        std::unordered_map<vertex_t, uint32_t> unique_vertices{};

        LOG_INFO("load shape count: {}",shapes.size());
        for (const auto &shape : shapes)
        {
            unique_vertices.clear();
            const size_t triangle_count = shape.mesh.indices.size() / 3;
            const size_t vertex_count = attrib.vertices.size() / 3;
            LOG_INFO("shape {}",shape.name);
            LOG_INFO("triangle count {0}", triangle_count);
            LOG_INFO("vertex count {0}", vertex_count);
            LOG_INFO("shape material count {}",shape.mesh.material_ids.size());
            model.mesh.emplace_back();
            auto& mesh = model.mesh.back();
            mesh.name = shape.name;

            for (const auto &index : shape.mesh.indices)
            {
                vertex_t vertex{};
                vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                              attrib.vertices[3 * index.vertex_index + 2]};
                vertex.n = {attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};
                vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                                    attrib.texcoords[2 * index.texcoord_index + 1]};
                if (unique_vertices.count(vertex) == 0)
                {
                    unique_vertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
                    mesh.vertices.push_back(vertex);
                }
                mesh.indices.push_back(unique_vertices[vertex]);
            }
            for(const auto& m_id:shape.mesh.material_ids){
                mesh.materials.push_back(m_id);
            }
        }
        return model;
    }



TRACER_END