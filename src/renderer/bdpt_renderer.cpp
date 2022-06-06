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
        Camera,
        AreaLight,
        EnvLight,
        Surface,
        Medium
    };


    class Vertex{
    public:
        Vertex(){}

        VertexType type;
        Spectrum accu_coef;
        //pdf_fwd in Camera Sub-path Vertex is not the same with in Light Sub-path
        real pdf_fwd = 0;//pdf per unit area from prev vertex to this vertex along camera to light
        real pdf_bwd = 0;//pdf per unit area from next vertex to this vertex along camera to light


        bool is_delta = false;

        struct SurfacePoint{
            Point3f pos;
            Normal3f n;
            Point2f uv;
            Vector3f wo;
            const BSDF* bsdf = nullptr;
            const Primitive* primitive = nullptr;
        };
        struct MediumPoint{
            Point3f pos;
            const BSDF* phase = nullptr;
            Vector3f wo;
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
            SurfacePoint surface_p;
            MediumPoint medium_p;
            CameraPoint camera_p;
            AreaLightPoint area_light_p;
            EnvLightPoint env_light_p;
        };

    };

    const BSDF* get_scattering_bsdf(const Vertex& v){
        if(v.type == VertexType::Surface){
            return v.surface_p.bsdf;
        }
        else if(v.type == VertexType::Medium){
            return v.medium_p.phase;
        }
        else{
            assert(false);
        }
    }
    Vector3f get_scattering_wo(const Vertex& v){
        if(v.type == VertexType::Surface){
            return v.surface_p.wo;
        }
        else if(v.type == VertexType::Medium){
            return v.medium_p.wo;
        }
        else{
            assert(false);
        }
    }

    Point3f get_scattering_pos(const Vertex& v){
        if(v.type == VertexType::Surface){
            return v.surface_p.pos;
        }
        else if(v.type == VertexType::Medium){
            return v.medium_p.pos;
        }
        else{
            assert(false);
        }
    }

    Vertex create_camera_vertex(const Point3f& pos,const Normal3f& n){
        Vertex v;
        v.type = VertexType::Camera;
        v.camera_p.pos = pos;
        v.camera_p.n = n;
        return v;
    }
    Vertex create_surface_vertex(
            const SurfaceIntersection& isect,
            const BSDF* bsdf
            ){
        Vertex v;
        v.type = VertexType::Surface;
        v.surface_p.pos = isect.pos;
        v.surface_p.n = isect.shading.n;
        v.surface_p.uv = isect.uv;
        v.surface_p.wo = isect.wo;
        v.surface_p.bsdf = bsdf;
        v.surface_p.primitive = isect.primitive;
        return v;
    }
    Vertex create_medium_vertex(){
        NOT_IMPL
        return {};
    }
    Vertex create_area_light_vertex(const Point3f& pos,const Normal3f& n,const Point2f& uv,const AreaLight* light){
        Vertex v;
        v.type = VertexType::AreaLight;
        v.area_light_p.pos = pos;
        v.area_light_p.n = n;
        v.area_light_p.uv = uv;
        v.area_light_p.light = light;
        return v;
    }
    Vertex create_env_light_vertex(const Vector3f& light_to_ref){
        Vertex v;
        v.type = VertexType::EnvLight;
        v.env_light_p.light_to_ref = light_to_ref;
        return v;
    }

    real pdf_solid_angle_to_area(real pdf_sa,const Point3f& pos,const Vertex& dst_v){
        switch (dst_v.type) {
            case VertexType::Camera : {
                Vector3f dir = dst_v.camera_p.pos - pos;
                real dist_cos = abs_dot(dst_v.camera_p.n, normalize(dir));
                real dist2 = dir.length_squared();
                return pdf_sa * dist_cos / dist2;
            }
            case VertexType::EnvLight : {
                return pdf_sa;
            }
            case VertexType::AreaLight : {
                Vector3f dir = dst_v.area_light_p.pos - pos;
                real dist_cos = abs_dot(dst_v.area_light_p.n, normalize(dir));
                real dist2 = dir.length_squared();
                return pdf_sa * dist_cos / dist2;
            }
            case VertexType::Surface : {
                Vector3f dir = dst_v.surface_p.pos - pos;
                real dist_cos = abs_dot(dst_v.surface_p.n,dir) / (dir.length());
                real dist2 = dir.length_squared();
                return pdf_sa * dist_cos / dist2;
            }
            case VertexType::Medium : {
                NOT_IMPL
                return 0;
            }
            default:
                assert(false);
                return 0;
        }
    }
    real pdf_from_to(const Vertex& from,const Vertex& to){
        auto from_pos = get_scattering_pos(from);

         Vector3f from_to_dir;
        switch (to.type) {
            case VertexType::Camera:
                from_to_dir = to.camera_p.pos - from_pos;
                break;
            case VertexType::Medium:
                from_to_dir = to.medium_p.pos - from_pos;
                break;
            case VertexType::Surface:
                from_to_dir = to.surface_p.pos - from_pos;
                break;
            case VertexType::AreaLight:
                from_to_dir = to.area_light_p.pos - from_pos;
                break;
            case VertexType::EnvLight:
                from_to_dir = -to.env_light_p.light_to_ref;
                break;
        }
        real pdf_sa = get_scattering_bsdf(from)->pdf(
                from_to_dir, get_scattering_wo(from));

        return pdf_solid_angle_to_area(pdf_sa,from_pos,to);
    }

    real correct_shading_normal(const SurfaceIntersection& isect,const Vector3f& wo,const Vector3f& wi,
                                TransportMode mode){
        if(mode == TransportMode::Importance){
            real num = abs_dot(wo,isect.shading.n) * abs_dot(wi,isect.n);
            real denom = abs_dot(wo,isect.n) * abs_dot(wi,isect.shading.n);
            if(denom == 0) return 0;
            return num / denom;
        }
        else
            return 1;
    }

    int generate_camera_subpath(const Scene& scene,Sampler& sampler,MemoryArena& arena,
                                const Ray& ray,Vertex* v_path,int v_max_cnt)
    {
        auto camera = scene.get_camera();
        const auto camera_we_ret = camera->eval_we(ray.o,ray.d);
        const auto camera_pdf_ret = camera->pdf_we(ray.o,ray.d);
        auto& camera_v = v_path[0];

        camera_v = create_camera_vertex(ray.o,camera_we_ret.n);
        camera_v.accu_coef = Spectrum(1 / camera_pdf_ret.pdf_pos);
        camera_v.pdf_fwd = camera_pdf_ret.pdf_pos;
        camera_v.is_delta = false;

        Spectrum accu_coef = camera_we_ret.we / (camera_pdf_ret.pdf_pos * camera_pdf_ret.pdf_dir);
        real pdf_fwd = camera_pdf_ret.pdf_dir;
        Ray r = ray;

        accu_coef *= abs_dot(camera_we_ret.n,ray.d);

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
                    new_v.pdf_fwd = pdf_fwd;
                    new_v.is_delta = false;
                }
                break;
            }

            //todo process medium

            //process surface
            {
                SurfaceShadingPoint shd_p = isect.material->shading(isect,arena);
                assert(shd_p.bsdf);
                //add surface vertex
                const real cos_isect = abs_dot(isect.shading.n,isect.wo);
                const real dist2 = distance_squared(r.o,isect.pos);
                const real pdf_area = pdf_fwd * cos_isect / dist2;
                auto& new_v = v_path[vertex_count++];
                new_v = create_surface_vertex(isect,shd_p.bsdf);
                new_v.accu_coef = accu_coef;
                new_v.pdf_fwd = pdf_area;
                new_v.is_delta = shd_p.bsdf->is_delta();

                //sample bsdf
                //todo add transmode
                auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,sampler.sample3());
                if(!bsdf_sample_ret.is_valid())
                    break;

                const real pdf_bwd = shd_p.bsdf->pdf(isect.wo,bsdf_sample_ret.wi);
                assert(vertex_count > 1);
                auto prev_v = v_path[vertex_count - 2];
                prev_v.pdf_bwd = pdf_solid_angle_to_area(pdf_bwd,isect.pos,prev_v);

                accu_coef *= bsdf_sample_ret.f * abs_dot(bsdf_sample_ret.wi,isect.shading.n) / bsdf_sample_ret.pdf;
                //todo ???
                accu_coef *= correct_shading_normal(isect,isect.wo,bsdf_sample_ret.wi,TransportMode::Radiance);
                pdf_fwd = bsdf_sample_ret.pdf;
                r = Ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);
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
        if(!light_emit_ret.radiance){
            return 0;
        }
        Spectrum accu_coef;
        real pdf_bwd;
        auto& init_v = v_path[0];
        if(light->as_area_light()){
            real init_pdf = light_emit_ret.pdf_pos * light_pdf;
            init_v = create_area_light_vertex(light_emit_ret.pos,light_emit_ret.n,light_emit_ret.uv,light->as_area_light());
            init_v.accu_coef = Spectrum(1 / init_pdf);
            init_v.pdf_bwd = light_emit_ret.pdf_pos * light_pdf;
            init_v.is_delta = false;

            real emit_cos = abs_dot(light_emit_ret.n,light_emit_ret.dir);
            accu_coef = light_emit_ret.radiance * emit_cos /
                    (init_pdf * light_emit_ret.pdf_dir);
            pdf_bwd = light_emit_ret.pdf_dir;
        }
        else if(light->as_environment_light()){
            real init_pdf = light_emit_ret.pdf_dir * light_pdf;

            init_v = create_env_light_vertex(light_emit_ret.dir);
            init_v.accu_coef = Spectrum(1 / init_pdf);
            init_v.pdf_bwd = light_emit_ret.pdf_dir* light_pdf;
            init_v.is_delta = false;

            accu_coef = light_emit_ret.radiance / (init_pdf * light_emit_ret.pdf_pos);
            pdf_bwd = light_emit_ret.pdf_pos;
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

            //todo process medium

            //process surface
            {
                SurfaceShadingPoint shd_p = isect.material->shading(isect,arena);
                assert(shd_p.bsdf);
                //add surface vertex
                const real cos_isect = abs_dot(isect.shading.n,isect.wo);
                const real dist2 = distance_squared(r.o,isect.pos);
                const real pdf_area = pdf_bwd * cos_isect / dist2;
                auto& new_v = v_path[vertex_count++];
                new_v = create_surface_vertex(isect,shd_p.bsdf);
                new_v.accu_coef = accu_coef;
                new_v.pdf_bwd = pdf_area;
                new_v.is_delta = shd_p.bsdf->is_delta();

                //sample bsdf
                //todo add transmode
                auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,sampler.sample3());
                if(!bsdf_sample_ret.is_valid())
                    break;

                const real pdf_fwd = shd_p.bsdf->pdf(isect.wo,bsdf_sample_ret.wi);
                assert(vertex_count > 1);
                auto prev_v = v_path[vertex_count - 2];
                prev_v.pdf_fwd = pdf_solid_angle_to_area(pdf_fwd,isect.pos,prev_v);

                accu_coef *= bsdf_sample_ret.f * abs_dot(bsdf_sample_ret.wi,isect.shading.n) / bsdf_sample_ret.pdf;
                //todo ???
                accu_coef *= correct_shading_normal(isect,isect.wo,bsdf_sample_ret.wi,TransportMode::Importance);
                pdf_bwd = bsdf_sample_ret.pdf;
                r = Ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);
            }
        }
        if(!light->as_area_light() && vertex_count > 1){
            v_path[1].pdf_bwd = light_emit_ret.pdf_pos;
        }

        return vertex_count;
    }

    Spectrum connect_bdpt_path()
    {
        NOT_IMPL
        return {};
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
    BDPTRenderer(const BDPTRendererParams& params): params(params){}

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
                const Point2f film_coord = {
                        (pixel.x + film_sample.u) / film_width,
                        (pixel.y + film_sample.v) / film_height
                };
                CameraSample camera_sample{film_coord,{lens_sample.u,lens_sample.v}};
                Ray ray;
                const auto ray_weight = scene_camera->generate_ray(camera_sample,ray);

                //reused for every pixel spp
                auto camera_subpath = arena.alloc<bdpt::Vertex>(params.max_camera_vertex_count + 2);
                auto light_subpath = arena.alloc<bdpt::Vertex>(params.max_light_vertex_count + 1);

                int camera_subpath_count = bdpt::generate_camera_subpath(scene,*sampler,arena,ray,
                                                                   camera_subpath,
                                                                   params.max_camera_vertex_count + 2);



                //todo create light distribution
                int light_subpath_count = bdpt::generate_light_subpath(scene,*sampler,arena,*scene_light_distribution,
                                                                       light_subpath,
                                                                       params.max_light_vertex_count + 1);

                Spectrum L(0);
                for(int t = 1; t <= camera_subpath_count; ++t){
                    for(int s = 0; s <= light_subpath_count; ++s){
                        const int t_s_len = t + s;
                        Spectrum Ld(0);
                        if(t_s_len < 2 || (t == 1 && s == 1))
                            continue;
                        //ignore invalid connection related to env light
                        if(t > 1 && s != 0 && camera_subpath[t - 1].type == bdpt::VertexType::EnvLight){
                            continue;
                        }

                        if(t == 2 && s == 0){
                            //todo no mis ???
                            const auto& camera_beg = camera_subpath[0];
                            const auto& camera_end = camera_subpath[1];
                            //env light
                            if(camera_end.type == bdpt::VertexType::EnvLight){
                                Ld = camera_end.accu_coef *
                                        scene.environment_light->light_emit(
                                                camera_beg.camera_p.pos,
                                                -camera_end.env_light_p.light_to_ref
                                                );
                            }
                            else{
                                if(camera_end.type != bdpt::VertexType::Medium
                                && camera_end.surface_p.primitive->as_area_light()){
                                    Ld = camera_end.accu_coef *
                                            camera_end.surface_p.primitive->as_area_light()->light_emit(
                                                    camera_end.surface_p.pos,camera_end.surface_p.n,
                                                    camera_end.surface_p.uv,
                                                    camera_beg.camera_p.pos - camera_end.surface_p.pos
                                                    );
                                }
                            }
                            L += Ld;
                            continue;
                        }

                        if(s == 0){
                            //just consider camera subpath as a complate path
                            //equal to path tracing
                            //camera subpath: [ ...... , a , b ]
                            //only valid if b is light
                            const bdpt::Vertex& a = camera_subpath[t - 2];
                            const bdpt::Vertex& b = camera_subpath[t - 1];
                            const Point3f a_pos = a.type == bdpt::VertexType::Surface ?
                                    a.surface_p.pos : a.medium_p.pos;
                            //env light
                            if(b.type == bdpt::VertexType::EnvLight){
                                Ld = scene.environment_light->light_emit(a_pos,-b.env_light_p.light_to_ref);
                                Ld *= b.accu_coef;
                            }
                            else if(b.type == bdpt::VertexType::Surface){
                                if(auto light = b.surface_p.primitive->as_area_light()){
                                    Ld = light->light_emit(b.surface_p.pos,b.surface_p.n,b.surface_p.uv,
                                                           a_pos - b.surface_p.pos);
                                    Ld *= b.accu_coef;
                                }
                            }
                        }
                        else if(t == 1){
                            //only one point on the camera lens connect to the light subpath
                            //sample a point on camera and connect it to the light path

                            auto& light_end = light_subpath[s - 1];
                            const Point3f light_end_pos = light_end.type == bdpt::VertexType::Surface ?
                                    light_end.surface_p.pos : light_end.medium_p.pos;

                            Point3f camera_pos = camera_subpath[0].camera_p.pos;
                            auto camera_we = scene.get_camera()->eval_we(
                                    camera_pos,light_end_pos - camera_pos
                                    );
                            if(!camera_we.we){
                                continue;
                            }

                            Point2f pixel_coord = {
                                    camera_we.film_coord.x * film_width,
                                    camera_we.film_coord.y * film_height
                            };

                            if(!inside(pixel_coord,(Bounds2f)film_bounds)){
                                continue;
                            }
                            if(!scene.visible(camera_pos,light_end_pos)){
                                continue;
                            }
                            //evaluate bsdf
                            Vector3f light_to_camera = normalize(camera_pos - light_end_pos);
                            const BSDF* bsdf = bdpt::get_scattering_bsdf(light_end);
                            const Spectrum f = bsdf->eval(
                                    light_to_camera,bdpt::get_scattering_wo(light_end)
                                    );
                            if(!f){
                                continue;
                            }

                            //evaluate G
                            real G = 1 / distance_squared(camera_pos,light_end_pos);
                            if(light_end.type == bdpt::VertexType::Surface){
                                G *= abs_dot(light_to_camera,light_end.surface_p.n);
                            }
                            G *= abs_dot(light_to_camera,camera_we.n);

                            //todo handle medium tr

                            //evaluate contribution
                            L = camera_we.we * G * f * light_end.accu_coef
                                    / camera_subpath[0].pdf_fwd;
                        }
                        else if(s == 1){
                            //only one point on the light
                            //sample a point on light and connect it to the camera path
                            auto& camera_end = camera_subpath[t - 1];
                            //only consider end vertex is surface or medium
                            if(camera_end.type == bdpt::VertexType::EnvLight){
                                continue;
                            }
                            const Point3f camera_end_pos = camera_end.type == bdpt::VertexType::Surface ?
                                    camera_end.surface_p.pos : camera_end.medium_p.pos;
                            const auto& light_v = light_subpath[0];
                            if(light_v.type == bdpt::VertexType::AreaLight){
                                if(!scene.visible(camera_end_pos,light_v.area_light_p.pos)){
                                    continue;
                                }

                                Vector3f camera_to_light = light_v.area_light_p.pos - camera_end_pos;
                                Spectrum light_radiance = light_v.area_light_p.light->light_emit(
                                        light_v.area_light_p.pos,
                                        light_v.area_light_p.n,
                                        light_v.area_light_p.uv,
                                        -camera_to_light
                                        );

                                auto bsdf = bdpt::get_scattering_bsdf(camera_end);
                                //todo add transport mode
                                Spectrum bsdf_f = bsdf->eval(camera_to_light,
                                                             bdpt::get_scattering_wo(camera_end));

                                //todo handle medium tr


                                if(camera_end.type == bdpt::VertexType::Surface){
                                    //???
                                    bsdf_f *= abs_dot(camera_end.surface_p.n, normalize(camera_to_light));
                                }

                                bsdf_f *= abs_dot(light_v.area_light_p.n, normalize(camera_to_light));

                                Ld = camera_end.accu_coef * bsdf_f * light_radiance
                                        / distance_squared(camera_end_pos,light_v.area_light_p.pos)
                                        / light_v.pdf_bwd;
                            }
                            else{
                                assert(light_v.type == bdpt::VertexType::EnvLight);

                                Vector3f camera_to_light = normalize(-light_v.env_light_p.light_to_ref);

                                Ray shadow_ray(camera_end_pos,camera_to_light,eps);
                                if(scene.intersect(shadow_ray))
                                    continue;
                                Spectrum light_radiance = scene.environment_light->light_emit(
                                        camera_end_pos,camera_to_light);

                                auto bsdf = bdpt::get_scattering_bsdf(camera_end);
                                Spectrum bsdf_f = bsdf->eval(
                                        camera_to_light,
                                        bdpt::get_scattering_wo(camera_end)
                                        );

                                if(camera_end.type == bdpt::VertexType::Surface){
                                    bsdf_f *= abs_dot(camera_end.surface_p.n,camera_to_light);
                                }
                                Ld = camera_end.accu_coef * bsdf_f * light_radiance / light_v.pdf_bwd;
                            }
                        }
                        else{
                            const auto& camera_end = camera_subpath[t - 1];
                            const auto& light_end = light_subpath[s - 1];
                            if(camera_end.type == bdpt::VertexType::EnvLight){
                                continue;
                            }
                            const auto camera_end_pos= bdpt::get_scattering_pos(camera_end);
                            const auto light_end_pos = bdpt::get_scattering_pos(light_end);
                            if(!scene.visible(camera_end_pos,light_end_pos)){
                                continue;
                            }

                            //evaluate bsdf
                            Vector3f camera_to_light = normalize(light_end_pos - camera_end_pos);
                            const auto camera_end_bsdf = bdpt::get_scattering_bsdf(camera_end);
                            Spectrum camera_bsdf_f = camera_end_bsdf->eval(
                                    camera_to_light,bdpt::get_scattering_wo(camera_end)
                                    );
                            if(!camera_bsdf_f){
                                continue;
                            }

                            const auto light_end_bsdf = bdpt::get_scattering_bsdf(light_end);
                            Spectrum light_bsdf_f = light_end_bsdf->eval(
                                    -camera_to_light,bdpt::get_scattering_wo(light_end)
                                    );
                            if(!light_bsdf_f){
                                continue;
                            }

                            //evaluate G
                            real G = 1 / distance_squared(camera_end_pos,light_end_pos);
                            if(camera_end.type == bdpt::VertexType::Surface){
                                G *= abs_dot(camera_end.surface_p.n,camera_to_light);
                            }

                            if(light_end.type == bdpt::VertexType::Surface){
                                G *= abs_dot(light_end.surface_p.n,-camera_to_light);
                            }
                            //todo handle medium tr


                            //evaluate contribution
                            //todo correct normal factor
                            Ld = camera_end.accu_coef * camera_bsdf_f * G *
                                    light_end.accu_coef * light_bsdf_f;
                        }

                        //compute mis weight for connection strategy
                        auto remap = [](real f){
                            return (!isfinite(f) || f <= 0) ? 1 : f;
                        };

                        auto compute_mis_weight = [&](
                                const bdpt::Vertex* camera, int t,
                                const bdpt::Vertex* light,  int s) ->real
                        {
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

                        };
                        real mis_weight = 0;
                        if(s == 0){
                            assert( t > 2);
                            // ... , a , b
                            auto& a = camera_subpath[t - 2];
                            auto& b = camera_subpath[t - 1];
                            ScopedAssignment<real> scoped_a_pdf_bwd;
                            ScopedAssignment<real> scoped_b_pdf_bwd;

                            if(b.type == bdpt::VertexType::Surface){
                                auto light = b.surface_p.primitive->as_area_light();
                                if(!light){
                                    mis_weight = 0;
                                }
                                else{
                                    const auto scene_light_pdf = scene_light_distribution->discrete_pdf(light_index[light]);
                                    const auto light_pdf = light->emit_pdf(
                                            b.surface_p.pos,b.surface_p.wo,(Vector3f)b.surface_p.n);
                                    scoped_b_pdf_bwd = {
                                            &b.pdf_bwd,
                                            scene_light_pdf * light_pdf.pdf_pos
                                    };
                                    scoped_a_pdf_bwd = {
                                            &a.pdf_bwd,
                                            bdpt::pdf_solid_angle_to_area(light_pdf.pdf_dir,
                                                                          b.surface_p.pos,a)
                                    };

                                    mis_weight = compute_mis_weight(camera_subpath,t,nullptr,0);
                                }
                            }
                            else if(b.type == bdpt::VertexType::EnvLight){
                                auto env = scene.environment_light.get();
                                auto scene_light_pdf = scene_light_distribution->discrete_pdf(light_index[env]);
                                const auto light_pdf = env->emit_pdf({},b.env_light_p.light_to_ref,{});
                                scoped_b_pdf_bwd = {
                                        &b.pdf_bwd,
                                        scene_light_pdf * light_pdf.pdf_dir
                                };
                                scoped_a_pdf_bwd = {
                                        &a.pdf_bwd,
                                        light_pdf.pdf_pos
                                };

                                mis_weight = compute_mis_weight(camera_subpath,t,nullptr,0);
                            }
                            else{
                                mis_weight = 0;
                            }
                        }
                        else if(s == 1){
                            // [ ... , a , b ] --- [ c ]
                            auto& a = camera_subpath[t - 2];
                            auto& b = camera_subpath[t - 1];
                            auto& c= light_subpath[0];

                            const Point3f b_pos = bdpt::get_scattering_pos(b);

                            ScopedAssignment<real> scope_a_pdf_bwd;
                            ScopedAssignment<real> scope_b_pdf_bwd;
                            ScopedAssignment<real> scope_c_pdf_fwd;

                            if(c.type == bdpt::VertexType::AreaLight){
                                const Vector3f b_to_c = c.area_light_p.pos - b_pos;

                                const auto emit_pdf = c.area_light_p.light->emit_pdf(
                                        c.area_light_p.pos,-b_to_c,(Vector3f)c.area_light_p.n);
                                scope_b_pdf_bwd = {
                                        &b.pdf_bwd,
                                        bdpt::pdf_solid_angle_to_area(emit_pdf.pdf_dir,c.area_light_p.pos,b)
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
                                Vector3f b_to_c = - c.env_light_p.light_to_ref;
                                auto emit_pdf = scene.environment_light->emit_pdf({},c.env_light_p.light_to_ref,{});
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
                            mis_weight = compute_mis_weight(camera_subpath,t,light_subpath,1);
                        }
                        else if(t == 1){
                            // [ a ] --- [ b , c , ... ]
                            auto& a = camera_subpath[0];
                            auto& b = light_subpath[s - 1];
                            auto& c = light_subpath[s - 2];

                            Point3f b_pos = bdpt::get_scattering_pos(b);

                            auto camera_we_ret = scene.get_camera()->pdf_we(
                                    a.camera_p.pos,b_pos - a.camera_p.pos);
                            if(camera_we_ret.pdf_pos <= 0 || camera_we_ret.pdf_dir <= 0){
                                mis_weight = 0;
                            }
                            else{
                                auto camera_v = bdpt::create_camera_vertex(a.camera_p.pos,a.camera_p.n);
                                camera_v.accu_coef = Spectrum(1);
                                camera_v.is_delta = false;
                                camera_v.pdf_fwd = camera_we_ret.pdf_pos;
                                camera_v.pdf_fwd = bdpt::pdf_from_to(b,camera_v);

                                ScopedAssignment<real> scope_b_pdf_fwd = {
                                        &b.pdf_fwd,
                                        bdpt::pdf_solid_angle_to_area(camera_we_ret.pdf_dir,a.camera_p.pos,b)
                                };

                                const real c_pdf_fwd_sa = bdpt::get_scattering_bsdf(b)->pdf(
                                        bdpt::get_scattering_wo(b),a.camera_p.pos - b_pos);
                                const real c_pdf_fwd = bdpt::pdf_solid_angle_to_area(c_pdf_fwd_sa,b_pos,c);

                                ScopedAssignment<real> scope_c_pdf_fwd = {
                                        &c.pdf_fwd,
                                        c_pdf_fwd
                                };
                                mis_weight = compute_mis_weight(&camera_v,1,light_subpath,t);
                            }
                        }
                        else{
                            auto &a = camera_subpath[s - 2];
                            auto &b = camera_subpath[s - 1];
                            auto &c = light_subpath[t - 1];
                            auto &d = light_subpath[t - 2];

                            const Point3f b_pos = bdpt::get_scattering_pos(b);
                            const Point3f c_pos = bdpt::get_scattering_pos(c);

                            const BSDF* b_bsdf = bdpt::get_scattering_bsdf(b);
                            const BSDF* c_bsdf = bdpt::get_scattering_bsdf(c);

                            const Vector3f b_to_c = normalize(c_pos - b_pos);
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
                            mis_weight = compute_mis_weight(camera_subpath,t,light_subpath,s);
                        }

                        Ld *= mis_weight;

                        if(t == 1){
                            const real weight = film.get_filter()->eval(film_sample.u,film_sample.v);
                            splat_image.at(pixel.x,pixel.y).add(weight * Ld);
                        }
                        else{
                            L += Ld;
                        }
                    }
                }
                film_tile->add_sample(film_coord,L);

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