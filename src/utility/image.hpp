//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_IMAGE_HPP
#define TRACER_IMAGE_HPP

#include "common.hpp"

TRACER_BEGIN

    template<typename T>
    class Image2D{
    public:
        Image2D():w(0),h(0),data(nullptr){}
        Image2D(int width,int height,const T* d = nullptr)
        :w(width),h(height),data(newBox<T[]>(width*height))//注意w和h在data之后初始化
        {
            if(d){
                std::copy(d,d+w*h,data.get());
            }
        }
        Image2D(Image2D&& other) noexcept
        :w(other.w),h(other.h),data(std::move(other.data))
        {

        }
        Image2D& operator=(Image2D&& other) noexcept{
            destroy();
            new(this) Image2D(std::move(other));
            other.destroy();
            return *this;
        }

        template<typename U>
        Image2D& operator*=(const U& u){
            for(int i = 0; i < w * h; ++i){
                data[i] *= u;
            }
            return *this;
        }

        T& operator()(int x,int y) const{
            return at(x,y);
        }

        T& at(int x,int y) const{
            return data[x + y * w];
        }

        void destroy(){
            if(data){
                data.reset();
                w = h = 0;
            }
        }

        T* get_raw_data(){
            return data.get();
        }
        const T* get_raw_data() const{
            return data.get();
        }
        int width() const {return w;}
        int height() const{return h;}

    private:
        Box<T[]> data;
        int w,h;
    };


    template<typename T>
    class Image3D{
    public:
    };

TRACER_END

#endif //TRACER_IMAGE_HPP
