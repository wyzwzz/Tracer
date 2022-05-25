//
// Created by wyz on 2022/5/18.
//

#ifndef TRACER_RENDER_HPP
#define TRACER_RENDER_HPP
#include <mutex>
#include "common.hpp"
#include "utility/geometry.hpp"
#include "core/filter.hpp"
#include "utility/image.hpp"
#include "core/spectrum.hpp"
#include "utility/logger.hpp"
TRACER_BEGIN

struct RenderTarget{
//todo image2d
    Image2D<Spectrum> color;
};


    class Film{
    public:
        struct TilePixel{
            Spectrum contrib_sum;
            real filter_weight_sum = 0;
        };
        class Tile{
        public:
            //use int int replace Bounds2i as interface exposed
            Tile(const Bounds2i& tile_bound,const RC<Filter>& filter)
            : tile_pixel_bound(tile_bound),filter(filter),
            tile_pixels(newBox<TilePixel[]>(tile_bound.area()))
            {

            }
            void add_sample(const Point2f& pos,Spectrum li,real sample_weight = 1.0){
                //todo check is li is infinite
                Point2f discrete_pos = pos - Point2f(0.5,0.5);
                Point2i p0 = (Point2i)ceil(discrete_pos - filter->radius());
                Point2i p1 = (Point2i)floor(discrete_pos + filter->radius()) + Point2i(1,1);
                p0 = max(p0,tile_pixel_bound.low);
                p1 = min(p1,tile_pixel_bound.high);
                for(int y = p0.y; y < p1.y; ++y){
                    for(int x = p0.x; x < p1.x; ++x){
                        Point2f offset = pos - Point2f(x+0.5,y+0.5);
                        real filter_weight = filter->eval(offset.x,offset.y);

                        auto& tile_pixel = get_pixel(Point2i(x,y));
                        tile_pixel.contrib_sum += li * filter_weight * sample_weight;
                        tile_pixel.filter_weight_sum += filter_weight;
                    }
                }
                //ok

            }
            Bounds2i get_pixel_bound() const{
                return tile_pixel_bound;
            }
            //传入的是相对于整个film的坐标
            TilePixel& get_pixel(const Point2i& p){
                Point2i dp = p - tile_pixel_bound.low;
                int offset = dp.x + dp.y * (tile_pixel_bound.high.x - tile_pixel_bound.low.x);
                return tile_pixels[offset];
            }
        private:
            //todo replace with Image2D
            Box<TilePixel[]> tile_pixels;
            //存储的是相对于整个film的相对坐标
            const Bounds2i tile_pixel_bound;
            RC<Filter> filter;
        };
        Film(const Point2i& res,const RC<Filter>& filter)
        : resolution(res),filter(filter),pixels(newBox<Pixel[]>(res.x * res.y))
        {}
        int width() const { return resolution.x; }
        int height() const { return resolution.y; }
        Box<Tile> get_film_tile(const Bounds2i& pixel_bounds){
            Point2f half_pixel(0.5,0.5);
            Bounds2f sample_bounds = (Bounds2f)pixel_bounds;
            //根据filter生成新的tile bounds 因此每一个tile之间存在padding
            Point2i low = (Point2i)ceil(sample_bounds.low - half_pixel - filter->radius());
            Point2i high = (Point2i)floor(sample_bounds.high - half_pixel + filter->radius()) + Point2i(1,1);
            Bounds2i film_bounds = get_film_bounds();
            Bounds2i tile_bounds = intersect(Bounds2i(low,high),film_bounds);
            return newBox<Tile>(tile_bounds,filter);
        }

        void merge_film_tile(const Box<Tile>& tile){
            static std::mutex merge_mtx;
            std::lock_guard<std::mutex> lk(merge_mtx);
            for(Point2i pixel:tile->get_pixel_bound()){
                const auto& tile_pixel = tile->get_pixel(pixel);
                auto& film_pixel = get_pixel(pixel);
                film_pixel.color += tile_pixel.contrib_sum;
                film_pixel.weight += tile_pixel.filter_weight_sum;
            }

        }
        Bounds2i get_film_bounds() const{
            return Bounds2i(Point2i(0,0),resolution);
        }

        void write_render_target(RenderTarget& render_target){
            render_target.color = Image2D<Spectrum>(resolution.x,resolution.y);
            for(int y = 0; y < resolution.y; ++y){
                for(int x = 0; x < resolution.x; ++x){
                    const auto& pixel = get_pixel({x,y});
                    if(pixel.weight > 0)
                        render_target.color.at(x,y) = pixel.color / pixel.weight;
                }
            }
        }

    private:
        Point2i resolution;
        RC<Filter> filter;


        struct Pixel{
            Spectrum color;
            real weight = 0;
        };
        Box<Pixel[]> pixels;
        Pixel& get_pixel(const Point2i& p){
            int offset = p.x + p.y * resolution.x;
            return pixels[offset];
        }
    };



TRACER_END

#endif //TRACER_RENDER_HPP
