#include "main.hpp"


 void run_test_bssrdf(const RenderParams& params){
     auto filter = create_gaussin_filter(params.filter.radius,params.filter.alpha);
     auto camera = create_thin_lens_camera((real)params.render_target_width/params.render_target_height,
                                           {params.camera.pos[0],params.camera.pos[1],params.camera.pos[2]},
                                           {params.camera.target[0],params.camera.target[1],params.camera.target[2]},
                                           {params.camera.up[0],params.camera.up[1],params.camera.up[2]},
                                           params.camera.fov,
                                           params.camera.lens_radius,params.camera.fov);
     std::vector<RC<Primitive>> primitives;
     std::vector<RC<Material>> materials;

     auto default_zero_texture = create_constant_texture2d(Spectrum(0));
     auto default_bssrdf = newRC<BSSRDFSurface>();
     auto bssrdf = create_normalized_diffusion_bssrdf_surface(
             create_constant_texture2d(Spectrum(0.8)),
             create_constant_texture2d(Spectrum(0.5)),
             create_constant_texture2d(Spectrum(1.5))
             );


     RC<Material> disney = create_disney(
             create_constant_texture2d(Spectrum(0.3,0.7,0.7)),//base color
             create_constant_texture2d(Spectrum(0.0)),//metallic
             create_constant_texture2d(Spectrum(0.0)),//roughness
             create_constant_texture2d(Spectrum(0.1)),//transmission
             create_constant_texture2d(Spectrum(0.0)),//transmission roughness
             create_constant_texture2d(Spectrum(1.2)),//ior,
             create_constant_texture2d(Spectrum(0)),//specular scale
             create_constant_texture2d(Spectrum(0)),//specular tint
             create_constant_texture2d(Spectrum(0)),//anisotropic
             create_constant_texture2d(Spectrum(0)),//sheen
             create_constant_texture2d(Spectrum(0)),//sheen tint
             create_constant_texture2d(Spectrum(0)),//clearcoat
             create_constant_texture2d(Spectrum(0)),//clearcoat gloss
             newBox<const NormalMapper>(nullptr),bssrdf
     );

     auto vacuum = create_vacuum();
     auto fog = create_homogeneous_medium({0.03,0.03,0.03},{0.3,0.3,0.3},0.8,25);
     MediumInterface mi;
     mi.inside = vacuum;
     mi.outside = vacuum;

     auto ball_primitive = create_geometric_primitive(create_sphere(1.6,Transform()),disney,mi,Spectrum(0));
     primitives.emplace_back(ball_primitive);

     auto bvh = create_bvh_accel(3);
     {
         AutoTimer timer("bvh build");
         bvh->build(std::move(primitives));
     }

     auto scene = create_general_scene(bvh);
     scene->set_camera(camera);

     std::string default_ibl_filename = "gray_pier_4k.hdr";
     if(params.ibl_file_name.empty())
         params.ibl_file_name = default_ibl_filename;
     if(!params.ibl_file_name.empty()){
         auto env_map = create_texture2d_from_file(params.ibl_file_name);
         auto ibl = create_ibl_light(env_map, rotate_x(0));
         scene->environment_light = ibl;
         scene->lights.push_back(ibl.get());
     }

     scene->prepare_to_render();

     auto renderer = create_pt_renderer(pt_params);

     AutoTimer timer("render","s");
     auto render_target = renderer->render(*scene.get(), Film({params.render_target_width, params.render_target_height}, filter));
     write_image_to_hdr(render_target.color, params.render_result_name+".hdr");
     LOG_INFO("write hdr...");
     auto gamma_corrector = create_gamma_corrector(1.0/2.2);
     auto aces_tone_mapper = create_aces_tone_mapper(1);
//    aces_tone_mapper->process(render_target);
     auto& imgf = render_target.color;
     Image2D<Color3b> imgu8(imgf.width(),imgf.height());
     real inv_gamma = 1.0 / 2.2;
     for(int i = 0; i < imgf.width(); i++){
         for(int j = 0; j < imgf.height(); j++){
             imgu8.at(i,j).x = std::clamp<int>(std::pow(imgf.at(i,j).r,inv_gamma) * 255,0,255);
             imgu8.at(i,j).y = std::clamp<int>(std::pow(imgf.at(i,j).g,inv_gamma) * 255,0,255);
             imgu8.at(i,j).z = std::clamp<int>(std::pow(imgf.at(i,j).b,inv_gamma) * 255,0,255);
         }
     }
     write_image_to_png(imgu8,params.render_result_name+".png");
     LOG_INFO("write png...");
     LOG_INFO("finish task");
 }
void run_scene_test(const RenderParams& params){
    auto filter = create_gaussin_filter(params.filter.radius,params.filter.alpha);
    auto camera = create_thin_lens_camera((real)params.render_target_width/params.render_target_height,
                                          {params.camera.pos[0],params.camera.pos[1],params.camera.pos[2]},
                                          {params.camera.target[0],params.camera.target[1],params.camera.target[2]},
                                          {params.camera.up[0],params.camera.up[1],params.camera.up[2]},
                                          params.camera.fov,
                                          params.camera.lens_radius,params.camera.fov);
    std::vector<RC<Primitive>> primitives;
    std::vector<RC<Material>> materials;

    auto default_zero_texture = create_constant_texture2d(Spectrum(0));
    auto default_bssrdf = newRC<BSSRDFSurface>();

    RC<Material> metal0 = create_disney(
            create_constant_texture2d(Spectrum(0.7)),
            create_constant_texture2d(Spectrum(0.3)),
            create_constant_texture2d(Spectrum(0.15)),
            create_constant_texture2d(Spectrum(0.5)),default_zero_texture, create_constant_texture2d(Spectrum(1.5)),
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,newBox<const NormalMapper>(nullptr),default_bssrdf
            );
    RC<Material> metal1 = create_disney(
            create_constant_texture2d(Spectrum(0.7)),
            create_constant_texture2d(Spectrum(0.6)),
            create_constant_texture2d(Spectrum(0.25)),
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,newBox<const NormalMapper>(nullptr),default_bssrdf
    );

    RC<Material> metal2 = create_disney(
            create_constant_texture2d(Spectrum(0.7)),
            create_constant_texture2d(Spectrum(1)),
            create_constant_texture2d(Spectrum(0.15)),
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,default_zero_texture,default_zero_texture,
            default_zero_texture,newBox<const NormalMapper>(nullptr),default_bssrdf
    );

    RC<Material> disney = create_disney(
            create_constant_texture2d(Spectrum(0.3,0.7,0.7)),//base color
            create_constant_texture2d(Spectrum(0.3)),//metallic
            create_constant_texture2d(Spectrum(0.3)),//roughness
            create_constant_texture2d(Spectrum(0.0)),//transmission
            create_constant_texture2d(Spectrum(0.3)),//transmission roughness
            create_constant_texture2d(Spectrum(1.5)),//ior,
            create_constant_texture2d(Spectrum(1)),//specular scale
            create_constant_texture2d(Spectrum(0)),//specular tint
            create_constant_texture2d(Spectrum(0)),//anisotropic
            create_constant_texture2d(Spectrum(0)),//sheen
            create_constant_texture2d(Spectrum(0)),//sheen tint
            create_constant_texture2d(Spectrum(0)),//clearcoat
            create_constant_texture2d(Spectrum(0)),//clearcoat gloss
            newBox<const NormalMapper>(nullptr),default_bssrdf
            );

    auto vacuum = create_vacuum();
    auto fog = create_homogeneous_medium({0.03,0.03,0.03},{0.3,0.3,0.3},0.8,25);
    MediumInterface mi;
    mi.inside = vacuum;
    mi.outside = vacuum;
//    auto ball_primitive0 = create_geometric_primitive(create_sphere(1.6, translate(Vector3f(-4,0,0))),metal0,mi,Spectrum());
//    primitives.emplace_back(ball_primitive0);
//    auto ball_primitive1 = create_geometric_primitive(create_sphere(1.6, translate(Vector3f(0,0,0))),metal1,mi,Spectrum());
//    primitives.emplace_back(ball_primitive1);
//    auto ball_primitive2 = create_geometric_primitive(create_sphere(1.6, translate(Vector3f(4,0,0))),metal2,mi,Spectrum());
//    primitives.emplace_back(ball_primitive2);
    auto ball_primitive = create_geometric_primitive(create_sphere(1.6,Transform()),disney,mi,Spectrum(0));
    primitives.emplace_back(ball_primitive);

    auto bvh = create_bvh_accel(3);
    {
        AutoTimer timer("bvh build");
        bvh->build(std::move(primitives));
    }

    auto scene = create_general_scene(bvh);
    scene->set_camera(camera);

    std::string default_ibl_filename = "gray_pier_4k.hdr";
    if(params.ibl_file_name.empty())
        params.ibl_file_name = default_ibl_filename;
    if(!params.ibl_file_name.empty()){
        auto env_map = create_texture2d_from_file(params.ibl_file_name);
        auto ibl = create_ibl_light(env_map, rotate_x(0));
        scene->environment_light = ibl;
        scene->lights.push_back(ibl.get());
    }

    scene->prepare_to_render();

    auto renderer = create_pt_renderer(pt_params);

    AutoTimer timer("render","s");
    auto render_target = renderer->render(*scene.get(), Film({params.render_target_width, params.render_target_height}, filter));
    write_image_to_hdr(render_target.color, params.render_result_name+".hdr");
    LOG_INFO("write hdr...");
    auto gamma_corrector = create_gamma_corrector(1.0/2.2);
    auto aces_tone_mapper = create_aces_tone_mapper(1);
//    aces_tone_mapper->process(render_target);
    auto& imgf = render_target.color;
    Image2D<Color3b> imgu8(imgf.width(),imgf.height());
    real inv_gamma = 1.0 / 2.2;
    for(int i = 0; i < imgf.width(); i++){
        for(int j = 0; j < imgf.height(); j++){
            imgu8.at(i,j).x = std::clamp<int>(std::pow(imgf.at(i,j).r,inv_gamma) * 255,0,255);
            imgu8.at(i,j).y = std::clamp<int>(std::pow(imgf.at(i,j).g,inv_gamma) * 255,0,255);
            imgu8.at(i,j).z = std::clamp<int>(std::pow(imgf.at(i,j).b,inv_gamma) * 255,0,255);
        }
    }
    write_image_to_png(imgu8,params.render_result_name+".png");
    LOG_INFO("write png...");
    LOG_INFO("finish task");
}
void run(const RenderParams& params){
    auto filter = create_gaussin_filter(params.filter.radius,params.filter.alpha);
    auto camera = create_thin_lens_camera((real)params.render_target_width/params.render_target_height,
                                          {params.camera.pos[0],params.camera.pos[1],params.camera.pos[2]},
                                          {params.camera.target[0],params.camera.target[1],params.camera.target[2]},
                                          {params.camera.up[0],params.camera.up[1],params.camera.up[2]},
                                          params.camera.fov,
                                          params.camera.lens_radius,params.camera.fov);
    auto model = load_model_from_file(params.obj_file_name);
    std::vector<RC<Primitive>> primitives;
    LOG_INFO("load model's mesh count: {}",model.mesh.size());
    LOG_INFO("load model's material count: {}",model.material.size());
    std::vector<MaterialTexture> materials_res;
    std::vector<RC<Material>> materials;
    Span<const Light*> area_lights;
    for(auto& m:model.material){
        auto material = create_texture_from_file(m);
        materials_res.emplace_back(material);
        if(material.as_glass){
            materials.emplace_back(create_glass(
                    material.map_ks,
                    material.map_kt,
                    material.ior
                    ));
        }
        else{
            materials.emplace_back(create_phong_material(
                    material.map_ka,
                    material.map_kd,
                    material.map_ks,
                    material.map_ns));
        }
    }
    LOG_INFO("load material texture count: {}",materials_res.size());
    auto vacuum = create_vacuum();
    auto fog = create_homogeneous_medium({0.03,0.03,0.03},{0.3,0.3,0.3},0.8,25);
    for(const auto& mesh:model.mesh){
        assert(mesh.indices.size() % 3 == 0);
        const auto triangle_count = mesh.indices.size() / 3;
        assert(mesh.materials.size() == triangle_count);
        auto triangles = create_triangle_mesh(mesh,Transform());
        assert(triangles.size() == triangle_count);
        MediumInterface mi;
        mi.inside = vacuum;
        mi.outside = fog;
        for(size_t i = 0; i < triangle_count; ++i){
            //todo handle emission material
            assert(mesh.materials[i] < materials.size());
            const auto& material = materials[mesh.materials[i]];
            const auto& m_res = materials_res[mesh.materials[i]];
            if(materials_res[mesh.materials[i]].has_emission){
                primitives.emplace_back(create_geometric_primitive(triangles[i],material,mi,m_res.map_ke->evaluate(Point2f())));
                area_lights.emplace_back(primitives.back()->as_area_light());
            }
            else{
                primitives.emplace_back(create_geometric_primitive(triangles[i],material,mi,Spectrum()));
            }
        }
    }
    LOG_INFO("load primitives count: {}",primitives.size());
    auto bvh = create_bvh_accel(3);
    {
        AutoTimer timer("bvh build");
        bvh->build(std::move(primitives));
    }
    auto scene = create_general_scene(bvh);
    scene->lights = area_lights;
    scene->set_camera(camera);

    if(!params.ibl_file_name.empty()){
        auto env_map = create_texture2d_from_file(params.ibl_file_name);
        auto ibl = create_ibl_light(env_map, rotate_x(PIOver2_r));
        scene->environment_light = ibl;
        scene->lights.push_back(ibl.get());
    }
    scene->prepare_to_render();
    //create renderer
//    auto renderer = create_pt_renderer(pt_params);
//    auto renderer = create_sppm_renderer(sppm_params);
    auto renderer = create_bdpt_renderer(bdpt_params);

    AutoTimer timer("render","s");
    auto render_target = renderer->render(*scene.get(), Film({params.render_target_width, params.render_target_height}, filter));
    write_image_to_hdr(render_target.color, params.render_result_name+".hdr");
    LOG_INFO("write hdr...");
    auto gamma_corrector = create_gamma_corrector(1.0/2.2);
    auto aces_tone_mapper = create_aces_tone_mapper(1);
//    aces_tone_mapper->process(render_target);
    auto& imgf = render_target.color;
    Image2D<Color3b> imgu8(imgf.width(),imgf.height());
    real inv_gamma = 1.0 / 2.2;
    for(int i = 0; i < imgf.width(); i++){
        for(int j = 0; j < imgf.height(); j++){
            imgu8.at(i,j).x = std::clamp<int>(std::pow(imgf.at(i,j).r,inv_gamma) * 255,0,255);
            imgu8.at(i,j).y = std::clamp<int>(std::pow(imgf.at(i,j).g,inv_gamma) * 255,0,255);
            imgu8.at(i,j).z = std::clamp<int>(std::pow(imgf.at(i,j).b,inv_gamma) * 255,0,255);
        }
    }
    write_image_to_png(imgu8,params.render_result_name+".png");
    LOG_INFO("write png...");
    LOG_INFO("finish task");
}
int main(int argc,char** argv){
    RenderParams bedroom = {
        .render_result_name = "tracer_bedroom_pt_test",
        .filter = {.radius = 0.5,.alpha = 0.6},
        .render_target_width = 1280,
        .render_target_height = 720,
        .camera = {
                .pos = {3.456,1.212,3.299},
                .target = {2.699,1.195,2.645},
                .up = {-0.013,1.000,-0.011},
                .fov = PI_r * 39.4305 / 180.0,
                .lens_radius = 0,
                .focal_dist = 10.0
        },
        .obj_file_name = "bedroom.obj",
    };

    RenderParams cornell_box = {
        .render_result_name = "tracer_cornel-box_bdpt_vol_test",
        .filter = {.radius = 0.5,.alpha = 0.6},
        .render_target_width = 600,
        .render_target_height = 600,
        .camera = {
                .pos = {0,1,6.8},
                .target = {0,1,5.8},
                .up = {0,1,0},
                .fov = PI_r * 19.5 / 180.0,
                .lens_radius = 0,
                .focal_dist = 10.0
        },
        .obj_file_name = "cornell-box.obj"
    };

    RenderParams veach_mis = {
        .render_result_name = "tracer_veach-mis",
        .filter = {.radius = 0.5,.alpha = 0.6},
        .render_target_width = 1280,
        .render_target_height = 900,
        .camera = {
                .pos = {0.0,2.0,15.0},
                .target = {0.0,1.69521,14.0476},
                .up = {0.0,0.952421,-0.304787},
                .fov = PI_r * 27.3909 / 180.0,
                .lens_radius = 0,
                .focal_dist = 10.0
        },
        .obj_file_name = "veach-mis.obj"
    };
    RenderParams diningroom = {
            .render_result_name = "tracer_diningroom_bdpt_test_ts",
            .filter = {.radius = 0.5,.alpha = 0.6},
            .render_target_width = 450,
            .render_target_height = 270,
            .camera = {
                    .pos = {0.0,12.720,31.850},
                    .target = {0.0,12.546,30.865},
                    .up = {0.0,0.985,-0.174},
                    .fov = PI_r * 45 / 180.0,
                    .lens_radius = 0,
                    .focal_dist = 10.0
            },
            .obj_file_name = "diningroom.obj"
    };

    RenderParams cornellbox = {
            .render_result_name = "tracer_cornellbox_bdpt",
            .filter = {.radius = 0.5,.alpha = 0.6},
            .render_target_width = 600,
            .render_target_height = 600,
            .camera = {
                    .pos = {0,0,2.5},
                    .target = {0,0,0},
                    .up = {0,1,0},
                    .fov = PI_r * 60 / 180.0,
                    .lens_radius = 0,
                    .focal_dist = 10.0
            },
            .obj_file_name = "cornellbox.obj"
    };

    RenderParams test_ball = {
            .render_result_name = "tracer_ball_test",
            .filter = {.radius = 0.5,.alpha = 0.6},
            .render_target_width = 320,
            .render_target_height = 320,
            .camera = {
                    .pos = {0,-5,1},
                    .target = {0,0,0},
                    .up = {0,1,0},
                    .fov = PI_r * 48 / 180.0,
                    .lens_radius = 0,
                    .focal_dist = 10.0
            },
            .obj_file_name = "",
            .ibl_file_name = "gray_pier_4k.hdr"
    };
    RenderParams stanford_dragon = {
            .render_result_name = "tracer_dragon_test",
            .filter = {.radius = 0.5, .alpha = 0.6},
            .render_target_width = 600,
            .render_target_height = 600,
            .camera = {
                    .pos = {0,-5,1},
                    .target = {0,0,0},
                    .up = {0,1,0},
                    .fov = PI_r * 48 / 180.0,
                    .lens_radius = 0,
                    .focal_dist = 10.0
            },
            .obj_file_name = "dragon.obj",
            .ibl_file_name = "gray_pier_4k.hdr"
    };
    //FullBody_Decimated.obj
    RenderParams fullbody = {
            .render_result_name = "tracer_fullbody_test",
            .filter = {.radius = 0.5, .alpha = 0.6},
            .render_target_width = 600,
            .render_target_height = 600,
            .camera = {
                    .pos = {0,-5,1},
                    .target = {0,0,0},
                    .up = {0,1,0},
                    .fov = PI_r * 48 / 180.0,
                    .lens_radius = 0,
                    .focal_dist = 10.0
            },
            .obj_file_name = "fullbody.obj",
            .ibl_file_name = "gray_pier_4k.hdr"
    };
    RenderParams marry = {
            .render_result_name = "tracer_marry_test",
            .filter = {.radius = 0.5, .alpha = 0.6},
            .render_target_width = 1024,
            .render_target_height = 1024,
            .camera = {
                    .pos = {0,-5,1},
                    .target = {0,0,0},
                    .up = {0,1,0},
                    .fov = PI_r * 48 / 180.0,
                    .lens_radius = 0,
                    .focal_dist = 10.0
            },
            .obj_file_name = "Marry.obj",
            .ibl_file_name = "gray_pier_4k.hdr"
    };

    try{
//        run(cornell_box);
//        run_scene_test(test_ball);
//        run_test_bssrdf(stanford_dragon);
        run_disney_brdf(fullbody,disney_brdf_params);
    }
    catch(const std::exception& e){
        LOG_CRITICAL("exception: {}",e.what());
    }
    return 0;
}