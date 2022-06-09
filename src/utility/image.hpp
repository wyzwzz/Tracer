//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_IMAGE_HPP
#define TRACER_IMAGE_HPP

#include "common.hpp"
#include "utility/logger.hpp"
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
//                memcpy(data.get(),d,w*h*sizeof(T));
            }
            else{
                memset(data.get(),0,sizeof(T)*width*height);
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

        template<typename F>
        void map(F&& f){
            for(int i = 0; i < w * h; ++i){
                data[i] = f(data[i]);
            }
        }



        T& operator()(int x,int y) const{
            return at(x,y);
        }

        T& at(int x,int y) const{
            if(x < 0 || x >= w || y < 0 || y >= h){
                LOG_CRITICAL("invalid x y {} {}",x,y);
                throw std::out_of_range("invalid x y");
            }
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
    class MipMap2D{
    public:
        MipMap2D(){}
        explicit MipMap2D(const Image2D<T>& lod0_image){
            generate(lod0_image);
        }
        void generate(const Image2D<T>& lod0_image);

        int levels() const{
            return images.size();
        }
        bool valid() const{
            return levels() > 0;
        }
        const Image2D<T>& get_level(int level) const{
            assert(level >=0 && level <levels());
            return images[level];
        }
    private:
        std::vector<Image2D<T>> images;
    };

    template<typename T>
    void MipMap2D<T>::generate(const Image2D<T> &lod0_image) {
        this->images.clear();
        int last_w = lod0_image.width();
        int last_h = lod0_image.height();
        images.emplace_back(lod0_image);
        while(last_w > 1 && last_h > 1){
            if((last_w & 1) || (last_h & 1)){
                this->images.clear();
                throw std::runtime_error("invalid input image size: must be power of 2");
            }
            const int cur_w = last_w >> 1;
            const int cur_h = last_h >> 1;
            Image2D<T> cur_lod_image(cur_w,cur_h);
            auto& last_lod_image = images.back();
            for(int y = 0; y < cur_h; ++y){
                const int ly = y << 1;
                for(int x = 0; x < cur_w; ++x){
                    const int lx = x << 1;
                    const auto t00 = last_lod_image.at(lx,ly);
                    const auto t01 = last_lod_image.at(lx+1,ly);
                    const auto t10 = last_lod_image.at(lx,ly+1);
                    const auto t11 = last_lod_image.at(lx+1,ly+1);
                    cur_lod_image.at(x,y) = (t00 + t01 + t10 + t11) * 0.25;
                }
            }
            images.emplace_back(std::move(cur_lod_image));
            last_w >>= 1;
            last_h >>= 1;
        }
    }

    template<typename T>
    class Image3D{
    public:
    };

TRACER_END

#endif //TRACER_IMAGE_HPP
