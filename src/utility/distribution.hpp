//
// Created by wyz on 2022/5/27.
//

#ifndef TRACER_DISTRIBUTION_HPP
#define TRACER_DISTRIBUTION_HPP

#include "geometry.hpp"
#include <vector>
TRACER_BEGIN

struct Distribution1D{
public:
    Distribution1D(const real* f,int n)
    :func(f,f+n),cdf(n+1,0)
    {
        for(int i = 0; i < n; i++){
            cdf[i + 1] = cdf[i] + func[i] / n;// 1 / n as dx
        }
        func_int = cdf[n];
        if(!func_int){
            for(int i = 1; i < n + 1; i++) cdf[i] = real(i) / n;
        }
        else{
            for(int i = 1; i < n + 1; i++) cdf[i] /= func_int;
        }
    }
    int count() const { return func.size(); }
    real sample_continuous(real u,real* pdf,int* off = nullptr) const{
        //find first cdf[p] <= u
        //cdf[0] = 0
        auto p = std::lower_bound(cdf.rbegin(),cdf.rend(),u,std::greater<real>());

        int offset = cdf.size() - 1 - (p - cdf.rbegin());
        offset = std::clamp<int>(offset,0,cdf.size() - 2);
        if(off) *off = offset;
        real du = u - cdf[offset];
        if(offset + 1 < cdf.size() && cdf[offset + 1] - cdf[offset] > 0){
            du /= cdf[offset + 1] - cdf[offset];
        }

        //pdf is not p and could > 1 for continuous
        if(pdf){
            *pdf = (func_int > 0) ? func[offset] / func_int : 0;
        }
        return (offset + du) / count();
    }

    int sample_discrete(real u,real* pdf = nullptr,real* u_remapped = nullptr) const{
        assert(u >= 0);
        assert(u < 1);
        auto p = std::lower_bound(cdf.rbegin(),cdf.rend(),u,std::greater<real>());
        assert(p!=cdf.rend());
        int offset = cdf.size() - 1 -(p - cdf.rbegin());
        offset = std::clamp<int>(offset,0,cdf.size() - 2);
        if(pdf){
            *pdf = func_int > 0 ? func[offset] / (func_int * count()) : 0;
        }
        if(u_remapped){
            if(offset + 1 >= cdf.size()){
                *u_remapped = 0;
            }
            else
                *u_remapped = (u - cdf[offset]) / (cdf[offset + 1] - cdf[offset]);
        }
        return offset;
    }

    //index ~ [0,n)
    real discrete_pdf(int index) const{
        assert(index >=0 && index < func.size());
        return func[index] / (func_int * func.size());
    }


    std::vector<real> func,cdf;
    real func_int;
};

class Distribution2D{
public:
    Distribution2D(const real* data,int nu,int nv){
        pu_conditional_v.reserve(nv);
        for(int v = 0; v < nv; v++){
            pu_conditional_v.emplace_back(newBox<Distribution1D>(&data[v*nu],nu));
        }
        std::vector<real> marginal_func;
        marginal_func.reserve(nv);
        for(int v = 0; v < nv; v++){
            marginal_func.emplace_back(pu_conditional_v[v]->func_int);
        }
        pv_marginal = newBox<Distribution1D>(marginal_func.data(),nv);
    }
    std::pair<real,real> sample_continuous(real u,real v,float* pdf) const{
        float pdfs[2];
        int iv;
        real d1 = pv_marginal->sample_continuous(v,&pdfs[1],&iv);
        real d0 = pu_conditional_v[iv]->sample_continuous(u,&pdfs[0]);
        *pdf = pdfs[0] * pdfs[1];
        return {d0,d1};
    }
    real pdf(real u,real v) const{
        int iu = std::clamp<int>(u * pu_conditional_v[0]->count(),0,pu_conditional_v[0]->count() - 1);
        int iv = std::clamp<int>(v * pv_marginal->count(),0,pv_marginal->count()-1);
        return pu_conditional_v[iv]->func[iu] * pv_marginal->func_int;//p(u,v) = p(u|v) * p(v)
    }
private:
    std::vector<Box<Distribution1D>> pu_conditional_v;
    Box<Distribution1D> pv_marginal;
};

TRACER_END

#endif //TRACER_DISTRIBUTION_HPP
