//
// Created by wyz on 2022/6/1.
//
#include "core/renderer.hpp"
#include "core/scene.hpp"
#include "core/sampler.hpp"
#include "core/camera.hpp"
#include "core/light.hpp"
#include "core/sampling.hpp"
#include "core/material.hpp"
#include "core/primitive.hpp"
#include "utility/parallel.hpp"
#include "utility/hash.hpp"
#include "factory/renderer.hpp"
#include "direct_illumination.hpp"
#include <atomic>
TRACER_BEGIN



struct SPPMPixel{
    struct VisiblePoint{
        Point3f p;
        Vector3f wo;
        Spectrum coef;
        const BSDF* bsdf = nullptr;
        bool is_valid() const{
            return bsdf;
        }
    };

    SPPMPixel() = default;
    SPPMPixel(const SPPMPixel& p) noexcept{
        vp = p.vp;
        radius = p.radius;
        for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
            phi[i] = p.phi[i].load();
         M = p.M.load();
         N = p.N;
         tau = p.tau;
         direct_illum = p.direct_illum;
         total_count = p.total_count.load();
    }
    SPPMPixel& operator=(const SPPMPixel& p) noexcept{
        new(this) SPPMPixel(p);
        return *this;
    }

    VisiblePoint vp;


    real radius = 0;//todo ? 0.1
    Spectrum direct_illum;

    std::atomic<real> phi[SPECTRUM_COMPONET_COUNT] = {0};//todo ?
    std::atomic<int> M = 0;
    real N = 0;
    Spectrum tau;
    std::atomic<int> total_count = 0;
};




class VisiblePointContainer{
public:
    VisiblePointContainer(const Bounds3f& world_bounds,real grid_ele_len,size_t hash_table_count)
    :world_bound(world_bounds),grid_ele_len(grid_ele_len),hash_table_count(hash_table_count)
    {
        node_entries = newBox<std::atomic<SPPMPixelListNode*>[]>(hash_table_count);
        clear();
    }

    void clear(){
        for(size_t i = 0; i < hash_table_count; ++i){
            node_entries[i] = nullptr;
        }
    }

    void add_visible_point(SPPMPixel& pixel,MemoryArena& arena){
        Point3i low_grid = pos_to_grid(pixel.vp.p - (Vector3f)pixel.radius);
        Point3i high_grid = pos_to_grid(pixel.vp.p + (Vector3f)pixel.radius);
        for(int z = low_grid.z; z <= high_grid.z; ++z){
            for(int y = low_grid.y; y <= high_grid.y; ++y){
                for(int x = low_grid.x; x <= high_grid.x; ++x){
                    auto entry_id = grid_to_entry({x,y,z});
                    auto& entry = node_entries[entry_id];
                    auto node = arena.alloc_object<SPPMPixelListNode>();
                    node->sppm_pixel = &pixel;
                    node->next = entry;
                    while(!entry.compare_exchange_weak(node->next,node));
                }
            }
        }
    }

    /**
     * @param wi ray from light to isect
     */
    void add_photon(const Point3f& photon_pos,const Spectrum& phi,const Vector3f& wi){
        auto entry_index = pos_to_entry(photon_pos);
        for(SPPMPixelListNode* node = node_entries[entry_index]; node; node = node->next){
            auto& pixel = *node->sppm_pixel;
            if((photon_pos - pixel.vp.p).length_squared() > pixel.radius * pixel.radius)
                continue;
            Spectrum delta_phi = phi * pixel.vp.bsdf->eval(wi,pixel.vp.wo,TransportMode::Radiance);
            if(!delta_phi.is_finite())
                continue;

            for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i){
                atomic_add(pixel.phi[i],delta_phi[i]);
            }
            ++pixel.M;
            ++pixel.total_count;
        }
    }
private:
    Point3i pos_to_grid(const Point3f world_pos) const{
        auto offset = world_pos - world_bound.low;
        return Point3i(static_cast<int>(std::max<real>(offset.x,0) / grid_ele_len),
                       static_cast<int>(std::max<real>(offset.y,0) / grid_ele_len),
                       static_cast<int>(std::max<real>(offset.z,0) / grid_ele_len));
    }
    size_t grid_to_entry(const Point3i& grid_idx) const{
        const size_t low3_bits =
                ((grid_idx.x & 1) << 0) | ((grid_idx.y & 1) << 1) | ((grid_idx.z & 1) << 2);
        const size_t hash_val = hash(grid_idx.x, grid_idx.y, grid_idx.z);
        return ((hash_val << 3) % hash_table_count) | low3_bits;
    }
    size_t pos_to_entry(const Point3f& world_pos) const{
        return grid_to_entry(pos_to_grid(world_pos));
    }


    Bounds3f world_bound;
    real grid_ele_len;
    struct SPPMPixelListNode{
        SPPMPixel* sppm_pixel = nullptr;
        SPPMPixelListNode* next = nullptr;
    };
    Box<std::atomic<SPPMPixelListNode*>[]> node_entries;
    size_t hash_table_count;
};

class SPPMRenderer:public Renderer{
public:
    explicit SPPMRenderer(const SPPMRendererParams& params);

    RenderTarget render(const Scene& scene,Film film) override;

private:
    SPPMRendererParams params;
};

SPPMRenderer::SPPMRenderer(const SPPMRendererParams &params)
        : params(params)
{

}



RenderTarget SPPMRenderer::render(const Scene &scene, Film film) {
    auto world_bounds = scene.world_bounds();
    auto pixel_bounds = film.get_film_bounds();
    int pixel_count = pixel_bounds.area();
    Image2D<SPPMPixel> sppm_pixels(film.width(),film.height());


    real init_search_radius = params.init_search_radius;
    if(init_search_radius <= 0){
        init_search_radius = (world_bounds.high - world_bounds.low).length() / 1000;
    }
    world_bounds.low -= Vector3f(init_search_radius);
    world_bounds.high += Vector3f(init_search_radius);

    for(int y = 0; y < film.height(); ++y){
        for(int x = 0; x < film.width(); ++x){
            sppm_pixels(x,y).radius = init_search_radius;
        }
    }
    const int thread_count = actual_worker_count(params.worker_count);
    auto sampler_prototype = newRC<SimpleUniformSampler>(42,false);
    PerThreadNativeSamplers perthread_sample(thread_count,*sampler_prototype);

    std::vector<MemoryArena> perthread_vp_arenas(thread_count);

    auto scene_camera = scene.get_camera();

    auto scene_light_distribution = compute_light_power_distribution(scene);

    const int film_width = film.width();
    const int film_height = film.height();
    real max_radius = init_search_radius;
    for(int iter = 0; iter < params.iteration_count; ++iter){

        // generate SPPM visible points
        const real grid_ele_len = max_radius * real(1.05);
        VisiblePointContainer vp_container(world_bounds,grid_ele_len,40960);

        parallel_for_2d(thread_count,film_width,film_height,
                        params.task_tile_size,params.task_tile_size,
                        [&](int thread_index,const Bounds2i& tile_bounds)
        {
            auto sampler = perthread_sample.get_sampler(thread_index);
            auto& arena = perthread_vp_arenas[thread_index];

            for(Point2i pixel:tile_bounds){
                const Sample2 film_sample = sampler->sample2();
                const Sample2 lens_sample = sampler->sample2();
                const Point2f film_coord = {
                        (pixel.x + film_sample.u) / film_width,
                        (pixel.y + film_sample.v) / film_height
                };
                CameraSample camera_sample{film_coord,{lens_sample.u,lens_sample.v}};
                Ray ray;
                const auto ray_weight = scene_camera->generate_ray(camera_sample,ray);
                assert(ray_weight == 1.f);
                Spectrum coef(1.f);
                coef *= ray_weight;

                auto& sppm_pixel = sppm_pixels(pixel.x,pixel.y);
                bool specular_bounce = false;
                for(int depth = 0; depth < params.ray_trace_max_depth; ++depth){
                    SurfaceIntersection isect;
                    bool found_intersection = scene.intersect_p(ray,&isect);

                    if(!found_intersection){
                        //todo test depth == 0 ?
                        if(auto light = scene.environment_light.get()){
                            sppm_pixel.direct_illum += coef * light->light_emit(ray.o,ray.d);
                        }
                        break;
                    }

                    const SurfaceShadingPoint shd_p = isect.material->shading(isect,arena);

                    if(depth == 0 || specular_bounce){
                        if(auto light = isect.primitive->as_area_light()){
                            sppm_pixel.direct_illum += coef * light->light_emit(isect,-ray.d);
                        }
                    }

                    //direct illumination
                    Spectrum next_direct_illum;
                    for(auto light:scene.lights){
                        next_direct_illum += sample_light(scene,light,isect,shd_p,*sampler);
                    }
                    next_direct_illum += sample_bsdf(scene,isect,shd_p,*sampler);

                    sppm_pixel.direct_illum += coef * next_direct_illum;

                    if(shd_p.bsdf->has_diffuse() || depth == params.ray_trace_max_depth - 1){
                        if(!coef.is_finite()){
                            break;
                        }
                        sppm_pixel.vp = {isect.pos,isect.wo,coef,shd_p.bsdf};
                        break;
                    }

                    if(depth == params.ray_trace_max_depth - 1) break;
                    const auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,TransportMode::Radiance,sampler->sample3());
                    if(!bsdf_sample_ret.f || bsdf_sample_ret.pdf < eps)
                        break;
                    specular_bounce = bsdf_sample_ret.is_delta;
                    coef *= bsdf_sample_ret.f / bsdf_sample_ret.pdf * abs_cos(bsdf_sample_ret.wi,isect.geometry_coord.z);
                    ray = Ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);

                    //todo apply rr?
                }
                // add visible points to grid
                if(sppm_pixel.vp.is_valid()){
                    vp_container.add_visible_point(sppm_pixel,arena);
                }
            }
        });

        // trace photon
        std::vector<MemoryArena> photon_arenas(thread_count);
        parallel_for_1d_grid(thread_count,params.photons_per_iteration,4096,
                             [&](int thread_index,int begin,int end)

        {
            auto sampler = perthread_sample.get_sampler(thread_index);
            auto& arena = photon_arenas[thread_index];
            for(int i = begin; i < end; ++i){
                //emit a photon
                real light_pdf;
                int light_index = scene_light_distribution->sample_discrete(sampler->sample1().u,&light_pdf);
                if(light_pdf == 0){
                    LOG_CRITICAL("invalid light pdf");
                }

                assert(light_index < scene.lights.size());
                const auto& light = scene.lights[light_index];
                if(!light)
                    break;
                const auto emit = light->sample_le(sampler->sample5());
                if(emit.radiance.is_back()){
                    break;
                }
                Spectrum coef = emit.radiance * abs_dot(emit.n,emit.dir)
                        / (light_pdf * emit.pdf_pos * emit.pdf_dir);

                Ray ray(emit.pos,emit.dir,eps);
                //trace the photon emitted
                for(int depth = 1; depth <= params.photon_max_depth; ++depth){
                    if(!coef.is_finite()){
                        break;
                    }
                    SurfaceIntersection isect;
                    if(!scene.intersect_p(ray,&isect)){
                        break;
                    }
                    //add photon contribution to nearby visible points
                    //but ignore if this hit is direct illumination
                    if(depth > 1){
                        vp_container.add_photon(isect.pos,coef,isect.wo);
                    }

                    auto shd_p = isect.material->shading(isect,arena);
                    //todo importance sample
                    auto bsdf_sample_ret = shd_p.bsdf->sample(isect.wo,TransportMode::Importance,sampler->sample3());
                    if(bsdf_sample_ret.f.is_back() || bsdf_sample_ret.pdf < eps){
                        break;
                    }

                    coef *= bsdf_sample_ret.f *
                            abs_cos(bsdf_sample_ret.wi,isect.geometry_coord.z) / bsdf_sample_ret.pdf;
//                    if(coef.r > 1 || coef.g > 1 || coef.b > 1){
//                        LOG_CRITICAL("invalid coef");
//                        break;
//                    }
                    //apply russian roulette
                    if(depth >= params.photon_min_depth){
                        if(sampler->sample1().u > 0.9)
                            break;
                        coef /= 0.9;
                    }

                    ray = Ray(isect.eps_offset(bsdf_sample_ret.wi),bsdf_sample_ret.wi);
                }
                if(arena.used_bytes() > (4 << 20)){
                    arena.reset();
                }
            }

        });


        //update pixel
        for(int y = 0; y < film_height; ++y)
        {
            for(int x = 0; x < film_width; ++x)
            {
                if(sppm_pixels(x, y).vp.is_valid())
                {
                    max_radius = (std::max)(max_radius, sppm_pixels(x, y).radius);
                }
            }
        }
        if(max_radius == 0){
            max_radius = init_search_radius;
        }

        parallel_for_1d_grid(thread_count,film_height,16,
                             [&](int thread_index,int begin,int end)
        {
            for(int y = begin; y < end; ++y){
                for(int x = 0; x < film_width; ++x){
                    auto& pixel = sppm_pixels(x,y);
                    auto alpha = params.update_alpha;
                    if(pixel.vp.is_valid() && pixel.M > 0){
                        real new_N = pixel.N + alpha * pixel.M;
                        real new_R = pixel.radius * std::sqrt(new_N / (pixel.N + pixel.M));

                        Spectrum phi;
                        for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                            phi[i] = pixel.phi[i];
                        if(pixel.radius == 0){
                            LOG_CRITICAL("invalid radius");
                        }
                        pixel.tau = (pixel.tau + pixel.vp.coef * phi) * (new_R * new_R) / (pixel.radius * pixel.radius);
                        pixel.N = new_N;
                        pixel.radius = new_R;
                        pixel.M = 0;
                        for(int i = 0; i < SPECTRUM_COMPONET_COUNT; ++i)
                            pixel.phi[i] = 0;
                    }
                    pixel.vp.coef = Spectrum(0);
                    pixel.vp.bsdf = nullptr;
                }
            }
        });

        // reset memory arenas
        for(auto& arena:perthread_vp_arenas){
            arena.reset();
        }
    }

    RenderTarget ret;
    size_t photon_count = (size_t)params.iteration_count * params.photons_per_iteration;
    int direct_illum_count = params.iteration_count;
    ret.color = Image2D<Spectrum>(film_width,film_height);
    for(int y = 0; y < film_height; ++y){
        for(int x = 0; x < film_width; ++x){
            auto& pixel = sppm_pixels(x,y);
            Spectrum direct_illum = pixel.direct_illum / (real)direct_illum_count;
            real dem = photon_count * PI_r * pixel.radius * pixel.radius;
            Spectrum photon_illum = pixel.tau / dem;
            ret.color(x,y) =  direct_illum + photon_illum;//Spectrum(pixel.total_count * 1.0 / 500);
        }
    }
    return ret;
}


RC<Renderer> create_sppm_renderer(const SPPMRendererParams& params){
    return newRC<SPPMRenderer>(params);
}

TRACER_END
