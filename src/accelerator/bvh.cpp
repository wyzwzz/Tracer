//
// Created by wyz on 2022/5/19.
//
#include "core/aggregate.hpp"
#include "utility/geometry.hpp"
#include "utility/memory.hpp"
#include "utility/logger.hpp"
#include "core/primitive.hpp"
#include <algorithm>
#include <stack>
TRACER_BEGIN

namespace {

    struct BVHPrimitiveInfo{
        BVHPrimitiveInfo(){}
        BVHPrimitiveInfo(size_t index,const Bounds3f& bounds)
        :primitive_index(index),bounds(bounds),centroid(0.5 * bounds.low + 0.5 * bounds.high)
        {}
        size_t primitive_index = 0;
        Bounds3f bounds;
        Point3f centroid;
    };
    struct BVHBuildNode{
        void init_leaf(size_t first,size_t count,const Bounds3f& b){
            first_prim_offset = first;
            primitive_count = count;
            bounds = b;
            left = right = nullptr;

        }

        void init_interior(int axis,BVHBuildNode* l,BVHBuildNode* r){
            assert(l && r);
            left = l;
            right = r;
            split_axis = axis;
            bounds = Union(l->bounds,r->bounds);
            primitive_count = 0;
        }
        bool is_leaf_node() const{
            return primitive_count > 0;
        }
        Bounds3f bounds;
        BVHBuildNode* left = nullptr;
        BVHBuildNode* right = nullptr;
        int split_axis;
        size_t first_prim_offset;
        size_t primitive_count = 0;//if > 0 then it is a leaf node
    };
    struct LinearBVHNode{
        Bounds3f bounds;
        union {
            int primitive_offset;
            int second_child_offset;
        };
        uint16_t primitive_count;
        uint8_t axis;
        uint8_t pad[1];
        bool is_leaf_node() const{
            return primitive_count > 0;
        }
    };
    static_assert(sizeof(LinearBVHNode) == 32,"");

}

    //using SAH
    class BVHAccel: public Aggregate{
    public:

        BVHAccel(int max_leaf_prims):max_leaf_prims(max_leaf_prims){}

        ~BVHAccel() override{

        }

        void build(std::vector<RC<Primitive>> prims) override{
            if(prims.empty()) return;
            primitives = std::move(prims);
            size_t n = primitives.size();
            std::vector<BVHPrimitiveInfo> primitive_infos;
            primitive_infos.reserve(n);
            for(size_t i = 0; i < n; i++){
                primitive_infos.emplace_back(i,primitives[i]->world_bound());
            }

            MemoryArena arena(1<<20);
            size_t total_nodes_count = 0;
            std::vector<RC<Primitive>> ordered_prims;
            ordered_prims.reserve(n);

            BVHBuildNode* root = nullptr;

            root = recursive_build(primitive_infos,ordered_prims,0,n,total_nodes_count,arena);
            assert(root);
            primitives = std::move(ordered_prims);

            linear_nodes = alloc_aligned<LinearBVHNode>(total_nodes_count);
            size_t offset = 0;
            flatten_bvh_tree(root,offset);
            assert(offset == total_nodes_count);

            LOG_INFO("bvh tree build node count: {}",total_nodes_count);
        }

        virtual Bounds3f world_bound() const noexcept{
            if(!linear_nodes) return Bounds3f();
            return linear_nodes[0].bounds;
        }

        bool intersect(const Ray& ray) const noexcept override{
            if(!linear_nodes) return false;

            Vector3f inv_dir(1.0 / ray.d.x, 1.0 / ray.d.y, 1.0 / ray.d.z);
            int dir_is_neg[3] = {inv_dir.x < 0,inv_dir.y < 0,inv_dir.z < 0};

            std::stack<size_t> s;
            s.push(0);
            while(!s.empty()){
                assert(s.size() < 64);
                auto node_index = s.top();
                s.pop();
                const auto node = linear_nodes + node_index;
                if(!node->bounds.intersect_p(ray,inv_dir,dir_is_neg)) continue;
                if(node->is_leaf_node()){
                    for(int i = 0; i < node->primitive_count; i++){
                        if(primitives[node->primitive_offset + i]->intersect(ray)){
                            //if find one just return true
                            return true;
                        }
                    }
                }
                else{
                    if(dir_is_neg[node->axis]){
                        //second child first
                        s.push(node_index + 1);
                        s.push(node->second_child_offset);
                    }
                    else{
                        s.push(node->second_child_offset);
                        s.push(node_index + 1);
                    }
                }
            }
            return false;
        }

        bool intersect_p(const Ray& ray,SurfaceIntersection* isect) const noexcept override{
            if(!linear_nodes) return false;

            Vector3f inv_dir(1.0 / ray.d.x, 1.0 / ray.d.y, 1.0 / ray.d.z);
            int dir_is_neg[3] = {inv_dir.x < 0,inv_dir.y < 0,inv_dir.z < 0};
            bool hit = false;
            std::stack<size_t> s;
            s.push(0);
            while(!s.empty()){
                assert(s.size() < 64);
                auto node_index = s.top();
                s.pop();
                const auto node = linear_nodes + node_index;
                if(!node->bounds.intersect_p(ray,inv_dir,dir_is_neg)) continue;
                if(node->is_leaf_node()){
                    for(int i = 0; i < node->primitive_count; i++){
                        //note intersect_p will change ray.max_t which is mutable
                        //in order to find closet intersection
                        if(primitives[node->primitive_offset + i]->intersect_p(ray,isect)){
                            hit = true;
                        }
                    }
                }
                else{
                    if(dir_is_neg[node->axis]){
                        //second child first
                        s.push(node_index + 1);
                        s.push(node->second_child_offset);
                    }
                    else{
                        s.push(node->second_child_offset);
                        s.push(node_index + 1);
                    }
                }
            }
            return hit;
        }
    private:
        struct BucketInfo{
            size_t count = 0;
            Bounds3f bounds;
        };
        BVHBuildNode* recursive_build(std::vector<BVHPrimitiveInfo>& primitive_infos,
                                      std::vector<RC<Primitive>>& ordered_prims,
                                      size_t start,size_t end,
                                      size_t& total_nodes_count,MemoryArena& arena){
            assert(start < end);
            auto node = arena.alloc<BVHBuildNode>();
            ++total_nodes_count;
            Bounds3f bounds;
            for(size_t i = start; i < end; ++i)
                bounds = Union(bounds,primitive_infos[i].bounds);

            size_t primitives_count = end - start;

            auto insert_leaf = [&](){
                size_t first_prim_offset = ordered_prims.size();
                for(size_t i = start; i < end; ++i){
                    auto prim_index = primitive_infos[i].primitive_index;
                    ordered_prims.emplace_back(primitives[prim_index]);
                }
                node->init_leaf(first_prim_offset,primitives_count,bounds);
            };
            if(primitives_count == 1){
                insert_leaf();
            }
            else
            {
                Bounds3f centroid_bounds;
                for(size_t i = start; i < end; ++i)
                    centroid_bounds = Union(centroid_bounds,primitive_infos[i].centroid);

                int mid = (start + end) >> 1;
                int dim = centroid_bounds.maximum_extent();

                if(centroid_bounds.high[dim] == centroid_bounds.low[dim]){
                    insert_leaf();
                }
                else{
                    //using SAH
                    constexpr size_t SAH_PRIMITIVES_THRESHOLD = 2;
                    if(primitives_count <= SAH_PRIMITIVES_THRESHOLD){
                        std::nth_element(&primitive_infos[start],&primitive_infos[mid],
                                         &primitive_infos[end-1]+1,
                                         [dim](const BVHPrimitiveInfo& a,const BVHPrimitiveInfo& b){
                            return a.centroid[dim] < b.centroid[dim];
                        });
                    }
                    else
                    {
                        constexpr int N_BUCKETS = 12;
                        BucketInfo buckets[N_BUCKETS];

                        for(size_t i = start; i < end; ++i){
                            int b = N_BUCKETS * centroid_bounds.offset(primitive_infos[i].centroid)[dim];
                            if(b == N_BUCKETS) b = N_BUCKETS - 1;
                            assert(b >= 0 && b < N_BUCKETS);
                            buckets[b].count++;
                            buckets[b].bounds = Union(buckets[b].bounds,primitive_infos[i].bounds);
                        }
                        real cost[N_BUCKETS - 1];
                        //选择划分后包围盒面积更小的
                        for(int i = 0; i < N_BUCKETS - 1; ++i){
                            Bounds3f lb,rb;
                            int l_count = 0, r_count = 0;
                            for(int j = 0; j <= i; ++j){
                                lb = Union(lb,buckets[j].bounds);
                                l_count += buckets[j].count;
                            }
                            for(int j = i + 1; j < N_BUCKETS; ++j){
                                rb = Union(rb,buckets[j].bounds);
                                r_count += buckets[j].count;
                            }
                            cost[i] = 1 + (l_count * lb.surface_area() + r_count * rb.surface_area()) / bounds.surface_area();
                        }

                        auto min_bucket_pos = std::min_element(cost,cost+(N_BUCKETS)-1) - cost;
                        auto min_cost = cost[min_bucket_pos];

                        if(primitives_count > max_leaf_prims){
                            BVHPrimitiveInfo* p_mid = std::partition(
                                    &primitive_infos[start],&primitive_infos[end-1]+1,
                                    [=](const BVHPrimitiveInfo& prim_info){
                                        int b = N_BUCKETS * centroid_bounds.offset(prim_info.centroid)[dim];
                                        if(b == N_BUCKETS) b = N_BUCKETS - 1;
                                        return b <= min_bucket_pos;
                                    });
                            mid = p_mid - &primitive_infos[0];
                        }
                        else{
                            insert_leaf();
                            return node;
                        }
                    }
                    node->init_interior(dim, recursive_build(primitive_infos,ordered_prims,start,mid,total_nodes_count,arena),
                                        recursive_build(primitive_infos,ordered_prims,mid,end,total_nodes_count,arena));
                }
            }
            return node;
        }

        size_t flatten_bvh_tree(BVHBuildNode* node,size_t& offset){
            //将原来的BVH节点以深度搜索的顺序存储
            LinearBVHNode* linear_node = linear_nodes + offset;
            size_t node_offset = offset++;
            linear_node->bounds = node->bounds;
            if(node->is_leaf_node()){
                linear_node->primitive_count = node->primitive_count;
                linear_node->primitive_offset = node->first_prim_offset;
            }
            else{
                linear_node->axis = node->split_axis;
                linear_node->primitive_count = 0;
                flatten_bvh_tree(node->left,offset);
                linear_node->second_child_offset = flatten_bvh_tree(node->right,offset);
            }
            return node_offset;
        }
    private:

        const int max_leaf_prims;

        std::vector<RC<Primitive>> primitives;
        LinearBVHNode* linear_nodes = nullptr;
    };

    RC<Aggregate> create_bvh_accel(int max_leaf_primitives){
        return newRC<BVHAccel>(max_leaf_primitives);
    }

TRACER_END