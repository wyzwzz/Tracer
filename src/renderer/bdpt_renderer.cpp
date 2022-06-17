//
// Created by wyz on 2022/6/3.
//
#include "core/renderer.hpp"
#include "core/sampling.hpp"
#include "core/light.hpp"
#include "core/scene.hpp"
#include "core/spectrum.hpp"
#include "core/primitive.hpp"
#include "core/camera.hpp"
#include "core/material.hpp"
#include "core/sampler.hpp"
#include "utility/parallel.hpp"
#include "utility/hash.hpp"
#include "utility/misc.hpp"
#include "factory/renderer.hpp"
#include "direct_illumination.hpp"

TRACER_BEGIN

namespace bdpt{
    enum class VertexType{
        Camera ,
        AreaLight ,
        EnvLight ,
        Surface ,
        Medium
    };


    struct Vertex{
        Vertex(){}
#ifndef NDEBUG
        Vertex(const Vertex& v){
            type = v.type;
            accu_coef = v.accu_coef;
            pdf_fwd = v.pdf_fwd;
            pdf_bwd = v.pdf_bwd;
            is_delta = v.is_delta;
            medium_pt = v.medium_pt;
            surface_pt = v.surface_pt;
            camera_pt = v.camera_pt;
            area_light_pt = v.area_light_pt;
            medium_pt = v.medium_pt;
        }
        Vertex& operator=(const Vertex& v){
            type = v.type;
            accu_coef = v.accu_coef;
            pdf_fwd = v.pdf_fwd;
            pdf_bwd = v.pdf_bwd;
            is_delta = v.is_delta;
            medium_pt = v.medium_pt;
            surface_pt = v.surface_pt;
            camera_pt = v.camera_pt;
            area_light_pt = v.area_light_pt;
            medium_pt = v.medium_pt;

            return *this;
        }
#endif

        VertexType type = VertexType::Camera;

        Spectrum accu_coef;

        //pdf_fwd in Camera Sub-path Vertex is not the same with in Light Sub-path
        real pdf_fwd = 0;//pdf per unit area from prev vertex to this vertex along camera to light
        real pdf_bwd = 0;//pdf per unit area from next vertex to this vertex along camera to light

        bool is_delta = false;
        char pad[3]={0};

        struct SurfacePoint{
            Point3f pos;
            Normal3f n;//geometry normal
            Point2f uv;
            Vector3f wo;
            const BSDF* bsdf = nullptr;
            const Primitive* primitive = nullptr;
            const Medium* medium_outside = nullptr;
            const Medium* medium_inside = nullptr;
            const Medium* medium(const Vector3f& wo) const noexcept{
                return dot(wo,n) > 0 ? medium_outside : medium_inside;
            }
        };
        static_assert(sizeof(SurfacePoint) == 80,"");

        struct MediumPoint{
            Point3f pos;
            const BSDF* phase = nullptr;
            Vector3f wo;
            const Medium* medium = nullptr;
        };

        struct CameraPoint{
            Point3f pos;
            Normal3f n;
        };

        struct AreaLightPoint{
            Point3f pos;
            Normal3f n;
            Point2f uv;
            const AreaLight* light = nullptr;
        };

        struct EnvLightPoint{
            Vector3f light_to_ref;
        };
        union{
            MediumPoint    medium_pt;
            SurfacePoint   surface_pt;
            CameraPoint    camera_pt;
            AreaLightPoint area_light_pt;
            EnvLightPoint  env_light_pt;
        };

    };

    static_assert(sizeof(bool) == 1,"");
    static_assert(sizeof(Vertex) == 112,"");

    bool is_scattering_type(const Vertex& v){
        return v.type == VertexType::Surface || v.type == VertexType::Medium;
    }

    const BSDF* get_scattering_bsdf(const Vertex& v){
        assert(is_scattering_type(v));
        if(v.type == VertexType::Surface){
            return v.surface_pt.bsdf;
        }
        else if(v.type == VertexType::Medium){
            return v.medium_pt.phase;
        }
        else{
            return nullptr;
        }
    }
    Vector3f get_scattering_wo(const Vertex& v){
        assert(is_scattering_type(v));
        if(v.type == VertexType::Surface){
            return v.surface_pt.wo;
        }
        else if(v.type == VertexType::Medium){
            return v.medium_pt.wo;
        }
        else{
            return {};
        }
    }

    Point3f get_scattering_pos(const Vertex& v){
        assert(is_scattering_type(v));
        if(v.type == VertexType::Surface){
            return v.surface_pt.pos;
        }
        else if(v.type == VertexType::Medium){
            return v.medium_pt.pos;
        }
        else{
            return {};
        }
    }

    Vertex create_camera_vertex(const Point3f& pos,const Normal3f& n){
        Vertex v;
        v.type = VertexType::Camera;
        v.camera_pt.pos = pos;
        v.camera_pt.n = n;
        return v;
    }

    Vertex create_surface_vertex(
            const SurfaceIntersection& isect,
            const BSDF* bsdf)
    {
        Vertex v;
        v.type = VertexType::Surface;
        v.surface_pt.pos = isect.pos;
        v.surface_pt.n = (Normal3f)isect.geometry_coord.z;
        v.surface_pt.uv = isect.uv;
        v.surface_pt.wo = isect.wo;
        v.surface_pt.bsdf = bsdf;
        v.surface_pt.primitive = isect.primitive;
        v.surface_pt.medium_inside = isect.medium_inside;
        v.surface_pt.medium_outside = isect.medium_outside;
        return v;
    }

    Vertex create_surface_vertex(const Point3f& pos,const Normal3f& n,const Point2f& uv,
                                 const Vector3f& wo,const BSDF* bsdf,const Primitive* primitive)
    {
        Vertex v;
        v.type                 = VertexType::Surface;
        v.surface_pt.pos       = pos;
        v.surface_pt.n         = n;
        v.surface_pt.uv        = uv;
        v.surface_pt.wo        = wo;
        v.surface_pt.bsdf      = bsdf;
        v.surface_pt.primitive = primitive;
        return v;
    }

    Vertex create_medium_vertex(const Point3f& pos,const Vector3f& wo,const Medium* medium,const BSDF* phase){
        Vertex ret;
        ret.type = VertexType::Medium;
        ret.medium_pt.pos = pos;
        ret.medium_pt.wo = wo;
        ret.medium_pt.medium = medium;
        ret.medium_pt.phase = phase;
        return ret;
    }
    Vertex create_area_light_vertex(const Point3f& pos,const Normal3f& n,const Point2f& uv,const AreaLight* light){
        Vertex v;
        v.type                = VertexType::AreaLight;
        v.area_light_pt.pos   = pos;
        v.area_light_pt.n     = n;
        v.area_light_pt.uv    = uv;
        v.area_light_pt.light = light;
        return v;
    }
    Vertex create_env_light_vertex(const Vector3f& light_to_ref){
        Vertex v;
        v.type                      = VertexType::EnvLight;
        v.env_light_pt.light_to_ref = light_to_ref;
        return v;
    }

    real pdf_solid_angle_to_area(real pdf_sa,const Point3f& pos,const Vertex& dst_v){
        switch (dst_v.type) {
            case VertexType::Camera : {
                Vector3f dir  = dst_v.camera_pt.pos - pos;
                real dist_cos = abs_cos((Vector3f)dst_v.camera_pt.n, dir);
                real dist2    = dir.length_squared();
                return pdf_sa * dist_cos / dist2;
            }
            case VertexType::EnvLight : {
                return pdf_sa;
            }
            case VertexType::AreaLight : {
                Vector3f dir  = dst_v.area_light_pt.pos - pos;
                real dist_cos = abs_cos((Vector3f)dst_v.area_light_pt.n, dir);
                real dist2    = dir.length_squared();
                return pdf_sa * dist_cos / dist2;
            }
            case VertexType::Surface : {
                Vector3f dir  = dst_v.surface_pt.pos - pos;
                real dist_cos = abs_cos((Vector3f)dst_v.surface_pt.n,dir);
                real dist2    = dir.length_squared();
                return pdf_sa * dist_cos / dist2;
            }
            case VertexType::Medium : {
                return pdf_sa / distance_squared(dst_v.medium_pt.pos,pos);
            }
            default:
                LOG_ERROR("invalid type");
                assert(false);
                return 0;
        }
    }
    real pdf_from_to(const Vertex& from,const Vertex& to){
        auto from_pos = get_scattering_pos(from);

        Vector3f from_to_dir;
        switch (to.type) {
            case VertexType::Camera:
                from_to_dir = to.camera_pt.pos - from_pos;
                break;
            case VertexType::Medium:
                from_to_dir = to.medium_pt.pos - from_pos;
                break;
            case VertexType::Surface:
                from_to_dir = to.surface_pt.pos - from_pos;
                break;
            case VertexType::AreaLight:
                from_to_dir = to.area_light_pt.pos - from_pos;
                break;
            case VertexType::EnvLight:
                from_to_dir = -to.env_light_pt.light_to_ref;
                break;
            default:
                LOG_CRITICAL("invalid type");
        }
        real pdf_sa = get_scattering_bsdf(from)->pdf(from_to_dir.normalize(), get_scattering_wo(from));

        return pdf_solid_angle_to_area(pdf_sa,from_pos,to);
    }

    int generate_camera_subpath(const Scene& scene,Sampler& sampler,MemoryArena& arena,
                                const Ray& ray,Vertex* v_path,int v_max_cnt)
    {
        auto camera = scene.get_camera();
        //evaluate ray weight for camera and pdf
        const auto camera_we_ret = camera->eval_we(ray.o,ray.d);
        const auto camera_pdf_ret = camera->pdf_we(ray.o,ray.d);
        auto& camera_v = v_path[0];

        camera_v = create_camera_vertex(ray.o,camera_we_ret.n);
        camera_v.accu_coef = Spectrum(1 / camera_pdf_ret.pdf_pos);
        camera_v.pdf_fwd   = camera_pdf_ret.pdf_pos;
        camera_v.is_delta  = false;

        Spectrum accu_coef = camera_we_ret.we / (camera_pdf_ret.pdf_pos * camera_pdf_ret.pdf_dir);
        real pdf_fwd       = camera_pdf_ret.pdf_dir;
        Ray r = ray;

        accu_coef *= abs_cos((Vector3f)camera_we_ret.n,ray.d);

        int vertex_count = 1;

        while(vertex_count < v_max_cnt){
            SurfaceIntersection isect;
            bool found_intersect = scene.intersect_p(r,&isect);
            //todo record medium point even if not found intersection
            if(!found_intersect){
                if(auto light = scene.environment_light){
                    auto& new_v = v_path[vertex_count++];
                    new_v = create_env_light_vertex(-r.d);
                    new_v.accu_coef = accu_coef;
                    new_v.pdf_fwd   = pdf_fwd;
                    new_v.is_delta  = false;
                }
                break;
            }

            //process medium
            auto medium = isect.medium(isect.wo);
            auto medium_sample_ret = medium->sample(r.o,isect.pos,sampler,arena);
            accu_coef *= medium_sample_ret.throughout;
            if(medium_sample_ret.is_scattering_happened()){
                const auto& sp = medium_sample_ret.scattering_point;

                auto& new_v = v_path[vertex_count++];
                new_v = create_medium_vertex(sp.pos,sp.wo,sp.medium,medium_sample_ret.phase_func);
                new_v.accu_coef = accu_coef;
                new_v.pdf_fwd = pdf_fwd / distance_squared(sp.pos,r.o);
                new_v.is_delta = false;

                const auto phase_sample = medium_sample_ret.phase_func->sample(sp.wo,TransportMode::Radiance,sampler.sample3());
                assert(phase_sample.is_valid());
                const real pdf_bwd = medium_sample_ret.phase_func->pdf(isect.wo,phase_sample.wi);
                auto& last_v = v_path[vertex_count - 2];
                last_v.pdf_bwd = pdf_solid_angle_to_area(pdf_bwd,sp.pos,last_v);

                accu_coef *= phase_sample.f / phase_sample.pdf;
                pdf_fwd = phase_sample.pdf;
                r = Ray(sp.pos,phase_sample.wi);
            }
            else
            {
                //process surface
                SurfaceShadingPoint shd_p = isect.material->shading(isect,arena);
                assert(shd_p.bsdf);
                //add surface vertex
                const real cos_isect = abs_cos(isect.geometry_coord.z,isect.wo);
                const real dist2     = distance_squared(r.o,isect.pos);
                const real pdf_area  = pdf_fwd * cos_isect / dist2;

                auto& new_v = v_path[vertex_count++];
                new_v = create_surface_vertex(isect,shd_p.bsdf);
                new_v.accu_coef = accu_coef;
                new_v.pdf_fwd   = pdf_area;
                new_v.is_delta  = shd_p.bsdf->is_delta();

                //sample bsdf
                auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,TransportMode::Radiance,sampler.sample3());
                if(!bsdf_sample_ret.is_valid())
                    break;

                const real pdf_bwd = shd_p.bsdf->pdf(isect.wo,bsdf_sample_ret.wi);
                assert(vertex_count > 1);
                auto& prev_v = v_path[vertex_count - 2];
                prev_v.pdf_bwd = pdf_solid_angle_to_area(pdf_bwd,isect.pos,prev_v);

                accu_coef *= bsdf_sample_ret.f * abs_cos(bsdf_sample_ret.wi,isect.geometry_coord.z)
                           / bsdf_sample_ret.pdf;

                pdf_fwd = bsdf_sample_ret.pdf;
                r       = Ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);
            }
        }

        return vertex_count;
    }

    int generate_light_subpath(const Scene& scene,Sampler& sampler,MemoryArena& arena,
                               const Distribution1D& light_distr,Vertex* v_path,int v_max_cnt)
    {
        real light_pdf;
        const auto light_idx = light_distr.sample_discrete(sampler.sample1().u,&light_pdf);
        const auto light = scene.lights[light_idx];
        //sample light emit
        const auto light_emit_ret = light->sample_le(sampler.sample5());

        if(!light_emit_ret.is_valid()){
            return 0;
        }

        Spectrum accu_coef;
        real pdf_bwd;

        auto& init_v = v_path[0];
        if(light->as_area_light()){
            real init_pdf = light_emit_ret.pdf_pos * light_pdf;

            init_v = create_area_light_vertex(light_emit_ret.pos,light_emit_ret.n,light_emit_ret.uv,light->as_area_light());
            init_v.accu_coef = Spectrum(1 / init_pdf);
            init_v.pdf_bwd   = init_pdf;
            init_v.is_delta  = false;

            real emit_cos = abs_cos((Vector3f)light_emit_ret.n,light_emit_ret.dir);
            accu_coef = light_emit_ret.radiance * emit_cos /
                    (init_pdf * light_emit_ret.pdf_dir);
            pdf_bwd = light_emit_ret.pdf_dir;
        }
        else if(light->as_environment_light()){
            real init_pdf = light_emit_ret.pdf_dir * light_pdf;

            init_v = create_env_light_vertex(light_emit_ret.dir);
            init_v.accu_coef = Spectrum(1 / init_pdf);
            init_v.pdf_bwd = init_pdf;
            init_v.is_delta = false;

            accu_coef = light_emit_ret.radiance / (init_pdf * light_emit_ret.pdf_pos);
            real pdf = light->as_environment_light()->pdf(light_emit_ret.pos,-light_emit_ret.dir) * light_pdf;
            pdf_bwd   = pdf;
        }

        Ray r(light_emit_ret.pos,light_emit_ret.dir,eps);

        int vertex_count = 1;
        while(vertex_count < v_max_cnt){
            SurfaceIntersection isect;
            bool found_intersect = scene.intersect_p(r,&isect);
            //todo record medium point even if not found intersection
            if(!found_intersect){
                break;
            }

            const auto medium = isect.medium(isect.wo);
            const auto medium_sample_ret = medium->sample(r.o,isect.pos,sampler,arena);
            accu_coef *= medium_sample_ret.throughout;
            if(medium_sample_ret.is_scattering_happened()){
                auto& sp = medium_sample_ret.scattering_point;
                auto& new_v = v_path[vertex_count++];
                new_v = create_medium_vertex(sp.pos,isect.wo,sp.medium,medium_sample_ret.phase_func);
                new_v.accu_coef = accu_coef;
                new_v.pdf_bwd = pdf_bwd / distance_squared(r.o,sp.pos);
                new_v.is_delta = false;

                auto phase_sample_ret = medium_sample_ret.phase_func->sample(
                        sp.wo,TransportMode::Importance,sampler.sample3());
                real pdf_fwd = medium_sample_ret.phase_func->pdf(sp.wo,phase_sample_ret.wi);
                auto& last_v = v_path[vertex_count - 2];
                last_v.pdf_fwd = pdf_solid_angle_to_area(pdf_fwd,sp.pos,last_v);

                accu_coef *= phase_sample_ret.f / phase_sample_ret.pdf;
                pdf_bwd = phase_sample_ret.pdf;
                r = Ray(sp.pos,phase_sample_ret.wi);
            }
            else
            {
                //process surface
                SurfaceShadingPoint shd_p = isect.material->shading(isect,arena);
                assert(shd_p.bsdf);
                //add surface vertex
                const real cos_isect = abs_cos(isect.geometry_coord.z,isect.wo);
                const real dist2     = distance_squared(r.o,isect.pos);
                const real pdf_area  = pdf_bwd * cos_isect / dist2;

                auto& new_v = v_path[vertex_count++];
                new_v = create_surface_vertex(isect,shd_p.bsdf);
                new_v.accu_coef = accu_coef;
                new_v.pdf_bwd   = pdf_area;
                new_v.is_delta  = shd_p.bsdf->is_delta();

                //sample bsdf
                auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,TransportMode::Importance,sampler.sample3());
                if(!bsdf_sample_ret.is_valid())
                    break;

                const real pdf_fwd = shd_p.bsdf->pdf(isect.wo,bsdf_sample_ret.wi);
                assert(vertex_count > 1);
                auto& prev_v = v_path[vertex_count - 2];
                prev_v.pdf_fwd = pdf_solid_angle_to_area(pdf_fwd,isect.pos,prev_v);

                accu_coef *= bsdf_sample_ret.f * abs_cos(bsdf_sample_ret.wi,isect.geometry_coord.z) / bsdf_sample_ret.pdf;

                pdf_bwd = bsdf_sample_ret.pdf;
                r       = Ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);
            }
        }
        // Correct subpath sampling densities for infinite area lights
        if(light->as_environment_light() && vertex_count > 1){
            v_path[1].pdf_bwd = light_emit_ret.pdf_pos;
        }

        return vertex_count;
    }

    Spectrum t2_s0_path_contrib(const Scene& scene,const Vertex* camera_subpath){
        //just see direct area or env light illumination
        const auto& camera_beg = camera_subpath[0];
        const auto& camera_end = camera_subpath[1];

        if(camera_end.type == bdpt::VertexType::EnvLight){
            return camera_end.accu_coef *
                 scene.environment_light->light_emit(
                         camera_beg.camera_pt.pos,
                         -camera_end.env_light_pt.light_to_ref.normalize());
        }
        if(camera_end.type == bdpt::VertexType::Surface
           && camera_end.surface_pt.primitive->as_area_light()){
            // diffuse light
            return camera_end.accu_coef *
                 camera_end.surface_pt.primitive->as_area_light()->light_emit(
                         camera_end.surface_pt.pos,camera_end.surface_pt.n,
                         camera_end.surface_pt.uv,
                         (camera_beg.camera_pt.pos - camera_end.surface_pt.pos).normalize());
        }
        return {};
    }

    Spectrum tx_s0_path_contrib(const Scene& scene,const Vertex* camera_subpath,int t){
        //just consider camera subpath as a complate path
        //equal to path tracing
        //camera subpath: [ ...... , a , b ]
        //only valid if b is light
        assert(t > 2);
        const bdpt::Vertex& a = camera_subpath[t - 2];
        const bdpt::Vertex& b = camera_subpath[t - 1];
        const Point3f a_pos = a.type == bdpt::VertexType::Surface ?
                              a.surface_pt.pos : a.medium_pt.pos;

        //env light
        if(b.type == bdpt::VertexType::EnvLight){
            return b.accu_coef * scene.environment_light->light_emit(a_pos,-b.env_light_pt.light_to_ref.normalize());
        }
        if(b.type == bdpt::VertexType::Surface){
            if(auto light = b.surface_pt.primitive->as_area_light()){
                return b.accu_coef * light->light_emit(b.surface_pt.pos,b.surface_pt.n,b.surface_pt.uv,
                                                       (a_pos - b.surface_pt.pos).normalize());
            }
        }
        return {};
    }

    Spectrum tx_s1_path_contrib(const Scene& scene,const Vertex* camera_subpath,int t,
                                const Vertex* light_subpath,Sampler& sampler){
        assert( t >= 2);
        //path tracing
        //only one point on the light
        //sample a point on light and connect it to the camera path
        auto& camera_end = camera_subpath[t - 1];
        //only consider end vertex is surface or medium
        if(camera_end.type == bdpt::VertexType::EnvLight){
            return {};
        }

        const Point3f camera_end_pos = camera_end.type == bdpt::VertexType::Surface ?
                                       camera_end.surface_pt.pos : camera_end.medium_pt.pos;
        const auto& light_v = light_subpath[0];
        if(light_v.type == bdpt::VertexType::AreaLight) {
            if (!scene.visible(camera_end_pos, light_v.area_light_pt.pos)) {
                return {};
            }

            Vector3f camera_to_light = (light_v.area_light_pt.pos - camera_end_pos).normalize();
            Spectrum light_radiance = light_v.area_light_pt.light->light_emit(
                    light_v.area_light_pt.pos,
                    light_v.area_light_pt.n,
                    light_v.area_light_pt.uv,
                    -camera_to_light);

            auto bsdf = bdpt::get_scattering_bsdf(camera_end);

            Spectrum bsdf_f = bsdf->eval(camera_to_light,
                                         bdpt::get_scattering_wo(camera_end), TransportMode::Radiance);

            const auto medium = camera_end.type == VertexType::Surface ?
                    camera_end.surface_pt.medium(camera_to_light) : camera_end.medium_pt.medium;

            Spectrum tr = medium->tr(camera_end_pos,light_v.area_light_pt.pos,sampler);

            if (camera_end.type == bdpt::VertexType::Surface) {
                bsdf_f *= abs_cos((Vector3f)camera_end.surface_pt.n, camera_to_light);
            }

            bsdf_f *= abs_cos((Vector3f)light_v.area_light_pt.n, -camera_to_light);

            return camera_end.accu_coef * bsdf_f * light_radiance * tr
                   / distance_squared(camera_end_pos, light_v.area_light_pt.pos)
                   / light_v.pdf_bwd;
        }
        else{
            assert(light_v.type == bdpt::VertexType::EnvLight);
            Vector3f camera_to_light = (-light_v.env_light_pt.light_to_ref).normalize();

            Ray shadow_ray(camera_end_pos,camera_to_light,eps);
            if(scene.intersect(shadow_ray))
                return {};
            Spectrum light_radiance = scene.environment_light->light_emit(
                    camera_end_pos,camera_to_light);

            auto bsdf = bdpt::get_scattering_bsdf(camera_end);
            Spectrum bsdf_f = bsdf->eval(
                    camera_to_light,
                    bdpt::get_scattering_wo(camera_end),TransportMode::Radiance
            );

            if(camera_end.type == bdpt::VertexType::Surface){
                bsdf_f *= abs_cos((Vector3f)camera_end.surface_pt.n,camera_to_light);
            }
            return camera_end.accu_coef * bsdf_f * light_radiance / light_v.pdf_bwd;
        }
    }

    Spectrum t1_sx_path_contrib(const Scene& scene,const Vertex* camera_subpath,
                                const Vertex* light_subpath,int s,
                                const Bounds2f film_bounds,const Point2i film_size,
                                Point2f& film_pixel_coord,
                                Sampler& sampler){
        //only one point on the camera lens connect to the light subpath
        //sample a point on camera and connect it to the light path
        assert(s >= 2);
        auto& light_end = light_subpath[s - 1];
        const Point3f light_end_pos = light_end.type == bdpt::VertexType::Surface ?
                                      light_end.surface_pt.pos : light_end.medium_pt.pos;

        Point3f camera_pos = camera_subpath[0].camera_pt.pos;
        auto camera_we = scene.get_camera()->eval_we(
                camera_pos,light_end_pos - camera_pos
        );
        if(!camera_we.we){
            return {};
        }

        Point2f pixel_coord = {
                camera_we.film_coord.x * film_size.x,
                camera_we.film_coord.y * film_size.y
        };

        film_pixel_coord = pixel_coord;
        if(!inside(pixel_coord,(Bounds2f)film_bounds)){
            return {};
        }
        if(!scene.visible(camera_pos,light_end_pos)){
            return {};
        }
        //evaluate bsdf
        Vector3f light_to_camera = (camera_pos - light_end_pos).normalize();
        const BSDF* bsdf = bdpt::get_scattering_bsdf(light_end);
        const Spectrum f = bsdf->eval(
                light_to_camera,bdpt::get_scattering_wo(light_end),TransportMode::Importance);
        if(!f){
            return {};
        }

        //evaluate G
        real G = 1 / distance_squared(camera_pos,light_end_pos);
        if(light_end.type == bdpt::VertexType::Surface){
            G *= abs_cos((Vector3f)light_end.surface_pt.n,light_to_camera);
        }
        G *= abs_cos((Vector3f)camera_we.n,light_to_camera);

        auto medium = light_end.type == VertexType::Surface ?
                light_end.surface_pt.medium(light_to_camera) :
                light_end.medium_pt.medium;

        auto tr = medium->tr(camera_pos,light_end_pos,sampler);

        //evaluate contribution
        return camera_we.we * G * f * light_end.accu_coef * tr
             / camera_subpath[0].pdf_fwd;
    }

    Spectrum tx_sx_path_contrib(const Scene& scene,
                                const Vertex* camera_subpath,int t,
                                const Vertex* light_subpath,int s,
                                Sampler& sampler){
        assert(s >= 2 && t >= 2);
        const auto& camera_end = camera_subpath[t - 1];
        const auto& light_end  = light_subpath[s - 1];
        if(camera_end.type == bdpt::VertexType::EnvLight){
            return {};
        }

        const auto camera_end_pos= bdpt::get_scattering_pos(camera_end);
        const auto light_end_pos = bdpt::get_scattering_pos(light_end);
        if(!scene.visible(camera_end_pos,light_end_pos)){
            return {};
        }

        //evaluate bsdf
        Vector3f camera_to_light = (light_end_pos - camera_end_pos).normalize();
        const auto camera_end_bsdf = bdpt::get_scattering_bsdf(camera_end);

        Spectrum camera_bsdf_f = camera_end_bsdf->eval(
                camera_to_light,bdpt::get_scattering_wo(camera_end),TransportMode::Radiance);

        if(!camera_bsdf_f){
            return {};
        }

        const auto light_end_bsdf = bdpt::get_scattering_bsdf(light_end);

        Spectrum light_bsdf_f = light_end_bsdf->eval(
                -camera_to_light,bdpt::get_scattering_wo(light_end),TransportMode::Importance);

        if(!light_bsdf_f){
            return {};
        }

        //evaluate G
        real G = 1 / distance_squared(camera_end_pos,light_end_pos);
        if(camera_end.type == bdpt::VertexType::Surface){
            G *= abs_cos((Vector3f)camera_end.surface_pt.n,camera_to_light);
        }

        if(light_end.type == bdpt::VertexType::Surface){
            G *= abs_cos((Vector3f)light_end.surface_pt.n,-camera_to_light);
        }

        assert(camera_end.type != VertexType::Camera
        && camera_end.type != VertexType::AreaLight);
        auto medium = camera_end.type == VertexType::Surface ?
            camera_end.surface_pt.medium(camera_to_light) : camera_end.medium_pt.medium;

        Spectrum tr = medium->tr(camera_end_pos,light_end_pos,sampler);

        //evaluate contribution
        return camera_end.accu_coef * camera_bsdf_f * G * tr *
             light_end.accu_coef * light_bsdf_f;
    }
    inline real remap(real f){
        return (!isfinite(f) || f <= 0) ? 1 : f;
    };
    real compute_mis_weight(const Vertex* camera,int t,
                            const Vertex* light,int s){
        real sum_pdf = 1;
        real cur_pdf = 1;
        //process light subpath
        for(int i = s - 1; i >= 0; --i){
            cur_pdf *= remap(light[i].pdf_fwd) /
                       (light[i].is_delta ? 1 : remap(light[i].pdf_bwd));
            if(!light[i].is_delta && (i >= 1 && !light[i - 1].is_delta)){
                sum_pdf += cur_pdf;
            }
        }
        //process camera subpath
        cur_pdf = 1;
        for(int i = t - 1; i > 0; --i){
            cur_pdf *= remap(camera[i].pdf_bwd) /
                       (camera[i].is_delta ? 1 : remap(camera[i].pdf_fwd));
            if(!camera[i].is_delta && !camera[i - 1].is_delta){
                sum_pdf += cur_pdf;
            }
        }
        return 1 / sum_pdf;
    }

    real mis_weight_tx_s0(const Scene& scene,Vertex* camera_subpath,int t,
                          const Distribution1D* scene_light_distribution,
                          const std::unordered_map<const Light*,int>& light_index){
        assert( t > 2);
        // ... , a , b
        auto& a = camera_subpath[t - 2];
        auto& b = camera_subpath[t - 1];
        TempAssign scoped_a_pdf_bwd;
        TempAssign scoped_b_pdf_bwd;

        if(b.type == bdpt::VertexType::Surface){
            auto light = b.surface_pt.primitive->as_area_light();
            if(!light){
                return 0;
            }

            const auto scene_light_pdf = scene_light_distribution->discrete_pdf(light_index.at(light));
            const auto light_pdf = light->emit_pdf(
                    b.surface_pt.pos,b.surface_pt.wo,(Vector3f)b.surface_pt.n);
            //original pdf_bwd for b should be 0 or uninitialized
            assert(b.pdf_bwd == 0);
            scoped_b_pdf_bwd = {
                    &b.pdf_bwd,
                    scene_light_pdf * light_pdf.pdf_pos
            };
            //re-assign for a' pwd_pdf because consider b as an area light vertex not a surface vertex
            //pdf_dir for light may not same with for geometry primitive due to material/bsdf
            scoped_a_pdf_bwd = {
                    &a.pdf_bwd,
                    bdpt::pdf_solid_angle_to_area(light_pdf.pdf_dir,
                                                  b.surface_pt.pos,a)
            };

            return compute_mis_weight(camera_subpath,t,nullptr,0);

        }
        else if(b.type == bdpt::VertexType::EnvLight){
            auto env = scene.environment_light.get();
            auto scene_light_pdf = scene_light_distribution->discrete_pdf(light_index.at(env));
            const auto light_pdf = env->emit_pdf({},b.env_light_pt.light_to_ref,{});
            //b is on world sphere
            scoped_b_pdf_bwd = {
                    &b.pdf_bwd,
                    scene_light_pdf * light_pdf.pdf_dir
            };

            scoped_a_pdf_bwd = {
                    &a.pdf_bwd,
                    light_pdf.pdf_pos
            };

            return compute_mis_weight(camera_subpath,t,nullptr,0);
        }
        return 0;
    }

    real mis_weight_tx_s1(const Scene& scene,Vertex* camera_subpath,int t,Vertex* light_subpath){
        // [ ... , a , b ] --- [ c ]
        auto& a = camera_subpath[t - 2];
        auto& b = camera_subpath[t - 1];
        auto& c= light_subpath[0];

        const Point3f b_pos = bdpt::get_scattering_pos(b);

        ScopedAssignment<real> scope_a_pdf_bwd;
        ScopedAssignment<real> scope_b_pdf_bwd;
        ScopedAssignment<real> scope_c_pdf_fwd;

        if(c.type == bdpt::VertexType::AreaLight){
            const Vector3f b_to_c = normalize(c.area_light_pt.pos - b_pos);

            const auto emit_pdf = c.area_light_pt.light->emit_pdf(
                    c.area_light_pt.pos,-b_to_c,(Vector3f)c.area_light_pt.n);
            scope_b_pdf_bwd = {
                    &b.pdf_bwd,
                    bdpt::pdf_solid_angle_to_area(emit_pdf.pdf_dir,c.area_light_pt.pos,b)
            };

            const real a_pdf_bwd_sa = bdpt::get_scattering_bsdf(b)->pdf(
                    bdpt::get_scattering_wo(b),b_to_c);

            scope_a_pdf_bwd = {
                    &a.pdf_bwd,
                    bdpt::pdf_solid_angle_to_area(a_pdf_bwd_sa,b_pos,a)
            };
            scope_c_pdf_fwd = {
                    &c.pdf_fwd,
                    bdpt::pdf_from_to(b,c)
            };
        }
        else{
            assert(c.type == bdpt::VertexType::EnvLight);
            Vector3f b_to_c = -(c.env_light_pt.light_to_ref).normalize();
            auto emit_pdf = scene.environment_light->emit_pdf({},c.env_light_pt.light_to_ref,{});
            scope_b_pdf_bwd = {
                    &b.pdf_bwd,
                    emit_pdf.pdf_pos
            };

            real a_pdf_bwd_sa = bdpt::get_scattering_bsdf(b)->pdf(
                    bdpt::get_scattering_wo(b),b_to_c);
            scope_a_pdf_bwd = {
                    &a.pdf_bwd,
                    bdpt::pdf_solid_angle_to_area(a_pdf_bwd_sa,b_pos,a)
            };
            scope_c_pdf_fwd = {
                    &c.pdf_fwd,
                    bdpt::pdf_from_to(b,c)
            };
        }
        return compute_mis_weight(camera_subpath,t,light_subpath,1);
    }

    real mis_weight_t1_sx(const Scene& scene,Vertex* camera_subpath,Vertex* light_subpath,int s){
        // [ a ] --- [ b , c , ... ]
        auto& a = camera_subpath[0];
        auto& b = light_subpath[s - 1];
        auto& c = light_subpath[s - 2];

        Point3f b_pos = bdpt::get_scattering_pos(b);

        auto camera_we_ret = scene.get_camera()->pdf_we(
                a.camera_pt.pos,normalize(b_pos - a.camera_pt.pos));
        if(camera_we_ret.pdf_pos <= 0 || camera_we_ret.pdf_dir <= 0){
            return 0;
        }

        auto camera_v = bdpt::create_camera_vertex(a.camera_pt.pos,a.camera_pt.n);
        camera_v.accu_coef = Spectrum(1);
        camera_v.is_delta  = false;
        camera_v.pdf_fwd   = camera_we_ret.pdf_pos;
        camera_v.pdf_bwd   = bdpt::pdf_from_to(b,camera_v);

        ScopedAssignment<real> scope_b_pdf_fwd = {
                &b.pdf_fwd,
                bdpt::pdf_solid_angle_to_area(camera_we_ret.pdf_dir,a.camera_pt.pos,b)
        };

        const real c_pdf_fwd_sa = bdpt::get_scattering_bsdf(b)->pdf(
                bdpt::get_scattering_wo(b),normalize(a.camera_pt.pos - b_pos));
        const real c_pdf_fwd = bdpt::pdf_solid_angle_to_area(c_pdf_fwd_sa,b_pos,c);

        ScopedAssignment<real> scope_c_pdf_fwd = {
                &c.pdf_fwd,
                c_pdf_fwd
        };
        return compute_mis_weight(&camera_v,1,light_subpath,s);
    }

    real mis_weight_tx_sx(const Scene& scene,Vertex* camera_subpath,int t,
                          Vertex* light_subpath,int s){
        auto &a = camera_subpath[t - 2];
        auto &b = camera_subpath[t - 1];
        auto &c = light_subpath[s - 1];
        auto &d = light_subpath[s - 2];

        const Point3f b_pos = bdpt::get_scattering_pos(b);
        const Point3f c_pos = bdpt::get_scattering_pos(c);

        const BSDF* b_bsdf = bdpt::get_scattering_bsdf(b);

        const BSDF* c_bsdf = bdpt::get_scattering_bsdf(c);

        assert(b_bsdf && c_bsdf);

        const Vector3f b_to_c = (c_pos - b_pos).normalize();
        // [ ... , a , b ] --- [ ... , c , d ]
        // a.pdf_bwd
        // b -> a
        const real a_pdf_bwd_sa = b_bsdf->pdf(bdpt::get_scattering_wo(b),b_to_c);
        const real a_pdf_bwd = bdpt::pdf_solid_angle_to_area(
                a_pdf_bwd_sa,b_pos,a);
        ScopedAssignment<real> scope_a_pdf_bwd = {
                &a.pdf_bwd,a_pdf_bwd};
        // b.pdf_bwd
        // c -> b
        const real b_pdf_bwd = bdpt::pdf_from_to(c, b);
        ScopedAssignment<real> scope_b_pdf_bwd = {
                &b.pdf_bwd, b_pdf_bwd
        };

        // c.pdf_fwd
        // b -> c
        const real c_pdf_fwd = bdpt::pdf_from_to(b,c);
        ScopedAssignment<real> scope_c_pdf_fwd = {
                &c.pdf_fwd, c_pdf_fwd
        };

        // d.pdf_fwd
        // c -> d
        const real d_pdf_fwd_sa = c_bsdf->pdf(bdpt::get_scattering_wo(c),-b_to_c);
        const real d_pdf_fwd = bdpt::pdf_solid_angle_to_area(
                d_pdf_fwd_sa,c_pos,d);
        ScopedAssignment<real> scope_d_pdf_fwd = {
                &d.pdf_fwd, d_pdf_fwd};
        return compute_mis_weight(camera_subpath,t,light_subpath,s);
    }

    struct BDPTEvalParams{
        BDPTEvalParams(const Scene& scene,const Film& film,
                       const Distribution1D* distrib,
                       const std::unordered_map<const Light*,int> light_idx)
        :scene(scene),film(film),scene_light_distribution(distrib),light_index(light_idx)
        {}
        const Scene& scene;
        const Film& film;
        const Distribution1D* scene_light_distribution;
        const std::unordered_map<const Light*,int> light_index;
    };

    template<typename F>
    Spectrum evaluate_bdpt_path(const BDPTEvalParams& params,
                                Vertex* camera_subpath,int camera_v_cnt,
                                Vertex* light_subpath,int light_v_cnt,
                                Sampler& sampler,
                                F&& f)
    {
        auto& scene = params.scene;
        auto& film = params.film;

        Spectrum L;
        for(int t = 1; t <= camera_v_cnt; ++t) {
            for (int s = 0; s <= light_v_cnt; ++s) {
                const int t_s_len = t + s;
                if(t_s_len < 2 || (t == 1 && s == 1))
                    continue;
                //ignore invalid connection related to env light
                if(t > 1 && s != 0 && camera_subpath[t - 1].type == bdpt::VertexType::EnvLight){
                    continue;
                }

                if(t == 2 && s == 0){
                    Spectrum Ld = t2_s0_path_contrib(scene,camera_subpath);
                    assert(Ld.is_valid());
                    L += Ld;
                    continue;
                }

                if(t == 1){
                    Point2f film_pixel_coord;
                    Spectrum Ld = t1_sx_path_contrib(scene,camera_subpath,light_subpath,s,
                                                     (Bounds2f)film.get_film_bounds(),
                                                     {film.width(),film.height()},
                                                     film_pixel_coord,sampler);
                    assert(Ld.is_valid());
                    real weight = mis_weight_t1_sx(scene,camera_subpath,light_subpath,s);
                    //add to external place
                    if(Ld.is_meaningful())
                        f(film_pixel_coord,weight * Ld);
                }
                else if(s == 0){
                    Spectrum Ld = tx_s0_path_contrib(scene,camera_subpath,t);
                    assert(Ld.is_valid());
                    real weight = mis_weight_tx_s0(scene,camera_subpath,t,params.scene_light_distribution,params.light_index);
                    L += weight * Ld;
                }
                else if(s == 1){
                    Spectrum Ld = tx_s1_path_contrib(scene,camera_subpath,t,light_subpath,sampler);
                    assert(Ld.is_valid());
                    real weight = mis_weight_tx_s1(scene,camera_subpath,t,light_subpath);
                    L += weight * Ld;
                }
                else{
                    Spectrum Ld = tx_sx_path_contrib(scene,camera_subpath,t,light_subpath,s,sampler);
                    assert(Ld.is_valid());
                    real weight = mis_weight_tx_sx(scene,camera_subpath,t,light_subpath,s);
                    L += weight * Ld;
                }
            }
        }
        return L;
    }


    class AtomicSpectrum{
    public:
        std::atomic<real> channels[SPECTRUM_COMPONET_COUNT];
        AtomicSpectrum(){
            for(auto& c:channels)
                c = 0;
        }
        AtomicSpectrum(const AtomicSpectrum& s){
            for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                channels[i] = s.channels[i].load();
        }
        AtomicSpectrum& operator=(const AtomicSpectrum& s){
            for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                channels[i] = s.channels[i].load();
            return *this;
        }
        void add(const Spectrum& s){
            for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                atomic_add(channels[i],s[i]);
        }
        Spectrum to_spectrum() const{
            Spectrum ret;
            for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                ret[i] = channels[i];
            return ret;
        }
    };
}




class BDPTRenderer: public Renderer{
public:
    BDPTRenderer(const BDPTRendererParams& params): params(params){
        assert(params.max_camera_vertex_count >= 1 && params.max_light_vertex_count >= 0);
    }

    RenderTarget render(const Scene& scene,Film film) override;

private:
    BDPTRendererParams params;
    using SplatImage = Image2D<bdpt::AtomicSpectrum>;
};

RenderTarget BDPTRenderer::render(const Scene &scene, Film film){



    auto scene_light_distribution = compute_light_power_distribution(scene);
    std::unordered_map<const Light*,int> light_index;
    for(int i = 0; i < scene.lights.size(); ++i){
        light_index[scene.lights[i]] = i;
    }

    auto scene_camera = scene.get_camera();

    auto sampler_prototype = newBox<SimpleUniformSampler>(42, false);

    const int thread_count = actual_worker_count(params.worker_count);

    PerThreadNativeSamplers perthread_samplers(thread_count, *sampler_prototype);

    const int film_width = film.width();
    const int film_height = film.height();

    SplatImage splat_image(film_width,film_height);

    const auto film_bounds = film.get_film_bounds();

    const int spp = params.spp;

    parallel_for_2d(thread_count,film_width,film_height,params.task_tile_size,params.task_tile_size,
                    [&](int thread_index,const Bounds2i& tile_bounds)
    {
        MemoryArena arena;

        auto sampler = perthread_samplers.get_sampler(thread_index);

        auto film_tile = film.get_film_tile(tile_bounds);

        for(const Point2i& pixel:tile_bounds){

            for(int i = 0; i < spp; ++i){
                const Sample2 film_sample = sampler->sample2();
                const Sample2 lens_sample = sampler->sample2();
                const Point2f pixel_coord = {
                        (pixel.x + film_sample.u),
                        (pixel.y + film_sample.v)
                };
                const Point2f film_coord = {
                        (pixel.x + film_sample.u) / film_width,
                        (pixel.y + film_sample.v) / film_height
                };
                CameraSample camera_sample{film_coord,{lens_sample.u,lens_sample.v}};
                Ray ray;
                const auto ray_weight = scene_camera->generate_ray(camera_sample,ray);
                assert(ray_weight == 1);

                //reused for every pixel spp
                auto camera_subpath = arena.alloc<bdpt::Vertex>(params.max_camera_vertex_count);
                auto light_subpath = arena.alloc<bdpt::Vertex>(params.max_light_vertex_count);

                int camera_subpath_count = bdpt::generate_camera_subpath(scene,*sampler,arena,ray,
                                                                   camera_subpath,
                                                                   params.max_camera_vertex_count);



                //todo create light distribution
                int light_subpath_count = bdpt::generate_light_subpath(scene,*sampler,arena,*scene_light_distribution,
                                                                       light_subpath,
                                                                       params.max_light_vertex_count);

                Spectrum L(0);

                L = bdpt::evaluate_bdpt_path(bdpt::BDPTEvalParams(scene,film,scene_light_distribution.get(),light_index),
                                             camera_subpath,camera_subpath_count,
                                             light_subpath,light_subpath_count,*sampler,
                                             [&](const Point2f& coord,const Spectrum& v){
                    //process for t == 1

                    //add Ld to splat image because this Ld is not belong to this tile pixel
                    splat_image.at(std::min<int>(film_width - 1,coord.x),
                            std::min<int>(film_height - 1,coord.y)).add(v);
                });

                film_tile->add_sample(pixel_coord,L);

                arena.reset();
            }
        }
        film.merge_film_tile(film_tile);
    });
    RenderTarget ret;

    film.write_render_target(ret);

    for(int y = 0; y < film_height; ++y){
        for(int x = 0; x < film_width; ++x){
            ret.color(x,y) += splat_image.at(x,y).to_spectrum() * ( real(1) / spp);
        }
    }
    return ret;
}

RC<Renderer> create_bdpt_renderer(const BDPTRendererParams& params){
    return newRC<BDPTRenderer>(params);
}


TRACER_END