//
// Created by wyz on 2022/5/19.
//
#include "core/camera.hpp"
#include "core/sampling.hpp"
#include "utility/transform.hpp"
#include "core/spectrum.hpp"
TRACER_BEGIN

    class ThinLensCamera: public Camera{
    private:
        Transform camera_to_world;
        Point3f cam_pos;
        Vector3f dir;

        real focal_distance = 1;//透镜到焦屏幕的距离
        real lens_radius = 0;
        real focal_film_width = 1;
        real focal_film_height = 1;
        real area_focal_film = 1;
        real area_lens = 1;
    public:
        ThinLensCamera(real film_aspect,
                       const Point3f& cam_pos,const Point3f& target,const Vector3f& up,
                       real fov,real lens_radius,real focal_dist)
       {
            this->camera_to_world = inverse(look_at(cam_pos,target,up));
            this->cam_pos = cam_pos;
            this->dir = normalize(target-cam_pos);
            this->focal_distance = focal_dist;
            this->lens_radius = lens_radius;
            this->focal_film_height = 2 * focal_dist * std::tan(fov * 0.5);
            this->focal_film_width = film_aspect * focal_film_height;
            this->area_focal_film = this->focal_film_height * this->focal_film_width;
            this->area_lens = this->lens_radius > 0 ? lens_radius * lens_radius * PI_r: 1;
       }

        ~ThinLensCamera() override{}

        real generate_ray(const CameraSample& camera_sample,Ray& ray) const noexcept override{
            const Point3f focal_film_pos = {
                    //左手系下x方向一开始是0.5
                    (real(0.5) - camera_sample.p_film.x) * focal_film_width,
                    (real(0.5) - camera_sample.p_film.y) * focal_film_height,
                    focal_distance
            };

            const Point2f disk_sample = ConcentricSampleDisk({camera_sample.pr_lens.x,camera_sample.pr_lens.y});
            const Point3f lens_pos = Point3f(
                    lens_radius * disk_sample.x,lens_radius*disk_sample.y,0.0
                    );
            const Point3f pos_on_cam = camera_to_world(lens_pos);
            const Vector3f pos_to_out = camera_to_world(normalize(focal_film_pos - lens_pos));

            ray.o = pos_on_cam;
            ray.d = pos_to_out;

            return 1.f;
        }

        CameraEvalWeResult eval_we(const Point3f& pos_on_cam,const Vector3f& pos_to_out) const noexcept override{
            Point3f lens_pos = inverse(camera_to_world)(pos_on_cam);
            Vector3f local_dir = inverse(camera_to_world)(normalize(pos_to_out)).normalize();

            if(local_dir.z <= 0){
                return {};
            }

            Point3f focal_film_pos = lens_pos + (focal_distance / local_dir.z) * local_dir;

            Point2f film_coord = {
                    real(0.5) - focal_film_pos.x / focal_film_width,
                    real(0.5) - focal_film_pos.y / focal_film_height
            };

            real cos2_theta = local_dir.z * local_dir.z;
            real we = focal_distance * focal_distance / (area_focal_film * area_lens * cos2_theta * cos2_theta);
            CameraEvalWeResult ret;
            ret.film_coord = film_coord;
            ret.we = Spectrum(we);
            ret.n = (Normal3f)dir;
            return ret;
        }

        CameraPdfWeResult pdf_we(const Point3f& pos_on_cam,const Vector3f& pos_to_out) const noexcept override{
            Vector3f local_dir = inverse(camera_to_world)(normalize(pos_to_out)).normalize();

            if(local_dir.z <= 0)
                return { 1 / area_lens, 0};

            real cos_theta = local_dir.z;
            real pdf_dir = focal_distance * focal_distance / (area_focal_film * cos_theta * cos_theta * cos_theta);

            return { 1 / area_lens, pdf_dir};
        }

        CameraSampleWiResult sample_wi(const Point3f& ref,const Sample2& sample) const noexcept override{
            Point3f local_ref = inverse(camera_to_world)(ref);
            //sample lens
            Point2f disk_pos = ConcentricSampleDisk(sample);
            Point3f lens_pos = {disk_pos.x * lens_radius,disk_pos.y * lens_radius,0};

            Vector3f local_dir = normalize(local_ref - lens_pos);
            if(local_dir.z <= 0){
                return {};
            }
            Point3f pos_on_focal = lens_pos + (focal_distance / local_dir.z) * local_dir;
            //0 ~ 1
            Point2f film_pos = {
                    real(0.5) - pos_on_focal.x / focal_film_width,
                    real(0.5) - pos_on_focal.y / focal_film_height
            };

            CameraSampleWiResult ret;
            ret.n = (Normal3f)dir;
            ret.pos_on_cam = camera_to_world(lens_pos);
            ret.ref_to_pos = ret.pos_on_cam - ref;
            ret.we = eval_we(ret.pos_on_cam,-ret.ref_to_pos).we;
            ret.pdf = (ret.pos_on_cam - ref).length_squared() / (local_dir.z * area_lens);
            ret.film_coord = film_pos;
            return ret;
        }

    };




    RC<Camera> create_thin_lens_camera(real film_aspect,
                                       const Point3f& cam_pos,const Point3f& target,const Vector3f& up,
                                       real fov,real lens_radius,real focal_dist){
        return newRC<ThinLensCamera>(film_aspect,
                                     cam_pos,target,up,
                                     fov,lens_radius,focal_dist);
    }

TRACER_END