//
// Created by wyz on 2022/5/22.
//

#ifndef TRACER_AGGREGATE_BSDF_HPP
#define TRACER_AGGREGATE_BSDF_HPP

#include "core/bsdf.hpp"
#include "core/intersection.hpp"
#include "material/bxdf/bxdf.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN

    template<int MAX_BXDF_COUNT>
    class AggregateBSDF:public LocalBSDF{
    private:
        real weights[MAX_BXDF_COUNT];//用于控制bxdf被选择的概率
        const BXDF* bxdfs[MAX_BXDF_COUNT];
        int bxdfs_count = 0;

        Spectrum albedo;
    public:

        AggregateBSDF(const Coord& g_coord,const Coord& s_coord,const Spectrum& albedo)
        : LocalBSDF(g_coord,s_coord),albedo(albedo)
        {

        }

        void add_bxdf(const BXDF* bxdf,real weight){
            assert(bxdfs_count < MAX_BXDF_COUNT);
            weights[bxdfs_count] = weight;
            bxdfs[bxdfs_count++] = bxdf;
        }

        Spectrum eval(const Vector3f& wi,const Vector3f& wo,TransportMode mode) const override{
            if(cause_black_fringes(wi,wo))
                return evaluate_black_fringes(wi,wo);
            //transform to local coordinate
            Vector3f lwi = shading_coord.global_to_local(wi).normalize();
            Vector3f lwo = shading_coord.global_to_local(wo).normalize();
            if(!lwi.z || !lwo.z)//negative is ok
                return {};

            Spectrum ret;
            for(int i = 0; i < bxdfs_count; i++){
                ret += bxdfs[i]->evaluate(lwi,lwo,mode);
            }
            return ret;
        }

        BSDFSampleResult sample(const Vector3f& wo,TransportMode mode,const Sample3& sample) const override{
            if(cause_black_fringes(wo))
                return sample_black_fringes(wo,mode,sample);
            //compute normalized local wo
            const Vector3f lwo = shading_coord.global_to_local(wo).normalize();
            if(!lwo.z)
                return {};

            real weight_sum = 0;
            for(int i = 0; i < bxdfs_count; ++i){
                weight_sum += weights[i];
            }
            assert(weight_sum > 0);

            //todo replace with 一维区间分布
            real comp_sel = (sample.u - real(0.001)) * weight_sum;
            const BXDF* bxdf = nullptr;
            real weight = 0;
            for(int i = 0; i < bxdfs_count; i++){
                if(comp_sel <= weights[i]){
                    bxdf = bxdfs[i];
                    weight = weights[i];
                    break;
                }
                comp_sel -= weights[i];
            }
            if(!bxdf){
                LOG_CRITICAL("should not be");
                return {};
            }
            auto bxdf_sam_ret = bxdf->sample(lwo,mode,{sample.v,sample.w});
            if(!bxdf_sam_ret.is_valid())
                return {};

            bxdf_sam_ret.lwi = bxdf_sam_ret.lwi.normalize();
            bxdf_sam_ret.pdf *= weight;
            for(int i = 0; i < bxdfs_count; i++){
                if(bxdfs[i] == bxdf)
                    continue;
                bxdf_sam_ret.f += bxdfs[i]->evaluate(bxdf_sam_ret.lwi,lwo,mode);
                bxdf_sam_ret.pdf += bxdfs[i]->pdf(bxdf_sam_ret.lwi,lwo) * weights[i];
            }

            Vector3f wi = shading_coord.local_to_global(bxdf_sam_ret.lwi);
            real normal_corr = normal_correct_factor(geometry_coord,shading_coord,wi);

            return BSDFSampleResult{wi,bxdf_sam_ret.f * normal_corr,bxdf_sam_ret.pdf,false};
        }

        real pdf(const Vector3f& wi, const Vector3f& wo) const override{
            if(cause_black_fringes(wi,wo))
                return pdf_black_fringes(wi,wo);

            //transform to local coordinate
            Vector3f lwi = shading_coord.global_to_local(wi).normalize();
            Vector3f lwo = shading_coord.global_to_local(wo).normalize();
            if(!lwi.z || !lwo.z)
                return 0;

            real weight_sum = 0;
            real pdf = 0;
            for(int i = 0; i < bxdfs_count; i++){
                weight_sum += weights[i];
                pdf += weights[i] * bxdfs[i]->pdf(lwi,lwo);
            }
            return weight_sum > 0 ? pdf / weight_sum : 0;
        }

        bool is_delta() const override{
            return false;
        }

        bool has_diffuse() const override{
            for(int i = 0; i < bxdfs_count; ++i){
                if(bxdfs[i]->has_diffuse()){
                    return true;
                }
            }
            return false;
        }

        Spectrum get_albedo() const override{
            return albedo;
        }
    };

TRACER_END

#endif //TRACER_AGGREGATE_BSDF_HPP
