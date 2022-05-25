//
// Created by wyz on 2022/5/22.
//

#ifndef TRACER_AGGREGATE_BSDF_HPP
#define TRACER_AGGREGATE_BSDF_HPP

#include "core/bsdf.hpp"
#include "core/intersection.hpp"
#include "material/bxdf/bxdf.hpp"
TRACER_BEGIN

    template<int MAX_BXDF_COUNT>
    class AggregateBSDF:public BSDF{
    private:
        real weights[MAX_BXDF_COUNT];//用于控制bxdf被选择的概率
        const BXDF* bxdfs[MAX_BXDF_COUNT];
        int bxdfs_count = 0;

        const Normal3f ns,ng;
        const Vector3f ss,ts;
    public:

        AggregateBSDF(const SurfaceIntersection& isect)
        :ng(normalize(isect.n)),ns(normalize(isect.map_n)),ss(normalize(isect.dndu)),ts(normalize(isect.dndv))
        {

        }

        void add_bxdf(const BXDF* bxdf,real weight){
            assert(bxdfs_count < MAX_BXDF_COUNT);
            weights[bxdfs_count] = weight;
            bxdfs[bxdfs_count++] = bxdf;
        }

        Spectrum eval(const Vector3f& wi,const Vector3f& wo) const override{
            //todo process black fringes

            //transform to local coordinate
            Vector3f lwi = Vector3f(dot(ss,wi),
                                    dot(ts,wi),
                                    dot(ns,wi));
            Vector3f lwo = Vector3f(dot(ss,wo),
                                    dot(ts,wo),
                                    dot(ns,wo));
            if(lwi.z <= 0 || lwo.z <= 0)
                return {};

            Spectrum ret;
            for(int i = 0; i < bxdfs_count; i++){
                ret += bxdfs[i]->evaluate(lwi,lwo);
            }

            return ret;
        }

        BSDFSampleResult sample(const Vector3f& _wo,const Sample3& sample) const override{
            //todo process black fringes

            //compute normalized local wo
            auto wo = normalize(_wo);
            const Vector3f lwo = Vector3f(dot(ss,wo),
                                          dot(ts,wo),
                                          dot(wo,ns));
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
            auto bxdf_sam_ret = bxdf->sample(lwo,{sample.v,sample.w});
            if(!bxdf_sam_ret.is_valid())
                return {};

            bxdf_sam_ret.lwi = normalize(bxdf_sam_ret.lwi);
            bxdf_sam_ret.pdf *= weight;
            for(int i = 0; i < bxdfs_count; i++){
                if(bxdfs[i] == bxdf)
                    continue;
                bxdf_sam_ret.f += bxdfs[i]->evaluate(bxdf_sam_ret.lwi,lwo);
                bxdf_sam_ret.pdf += bxdfs[i]->pdf(bxdf_sam_ret.lwi,lwo) * weights[i];
            }

            Vector3f wi = ss * bxdf_sam_ret.lwi.x + ts * bxdf_sam_ret.lwi.y
                    + (Vector3f)ns * bxdf_sam_ret.lwi.z;
            BSDFSampleResult ret;
            ret.wi = normalize(wi);
            ret.pdf = bxdf_sam_ret.pdf;
            ret.is_delta = false;
            ret.f = bxdf_sam_ret.f;
            return ret;
        }

        real pdf(const Vector3f& wi, const Vector3f& wo) const override{
            //todo process black fringes

            //transform to local coordinate
            Vector3f lwi = Vector3f(dot(ss,wi),
                                    dot(ts,wi),
                                    dot(ns,wi));
            Vector3f lwo = Vector3f(dot(ss,wo),
                                    dot(ts,wo),
                                    dot(ns,wo));
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
    };

TRACER_END

#endif //TRACER_AGGREGATE_BSDF_HPP
