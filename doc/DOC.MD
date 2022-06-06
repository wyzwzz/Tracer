# Tracer
## 渲染器运行流程
    1.将一张film划分为多个tile，每一个tile由一个线程负责，tile在划分时不相交的，但是具体的tile在创建时会根据filter的半径进行校正
    2.每一个tile的渲染是单线程的，即依次渲染tile里的每一个pixel，每个tile创建一个arena，当一个pixel渲染完后reset arena
    3.每一个像素的渲染，首先从Camera生成光线开始，光线的生成涉及到对像素在film上的采样和相机透镜位置上的采样，暂时不考虑时间
    4.生成光线后，接着是对场景的求交，找到交点处的几何信息和反射模型
    5.交点处的几何信息
    6.根据交点的几何信息从Material中得到Shading相关的信息

## 主要的一些接口类
### filter
用于image reconstruction 目前只实现高斯过滤
### camera

### triangle
因为triangle一般以mesh的形式出现 会有十分多的triangle 所以多于triangle的存储需要特别设置
专门设置一个TriangleMesh类用于存放所有的mesh的顶点数据 在Triangle类中存放索引即可
但是这样子每一个Triangle对象会需要一个GeometricPrimitive包装 GeometricPrimitive虽然只需要存储指向材质和shape的ptr 但是也是不小的开销
另一种思路是 专门创建一个TriangleBVH的shape也是一种思路 这样子的话一个Triangle Mesh只需要一个GeometricPrimitive
pi = p0 + ui * dpdu + vi * dpdv
 u2 - u0, v2 - v0   dpdu     p2 - p0
                  *       =  
 u1 - u0, v1 - v0   dpdv     p1 - p0

### aggregate
#### BVH
使用启发式分割算法 一个包围盒里面的子包围的总面积占比应该尽可能少
沿着最大变化的轴 均匀分割成若干个Bucket 分别计算每个Primitive落在哪个Bucket
最后计算沿着N_BUCKET-1个轴分割时各自的cost
cost = 1 + left_count * left_bounds_area / total_bounds_area + right_count * right_bounds_area / total_bounds_area
另外在分割结束的时候 primitives会变成递归先序遍历得到的叶节点顺序
创建一个LinearNode数组 用于存储所有的节点 包括子节点和叶节点 循序是递归先序遍历 等价于DFS
每个节点需要存储第二个子节点在数组中的偏移 默认下一个子节点就是当前节点+1
因为BVH树是必定同时有左右节点的
想要找到最近的交点 可以在每一次找到交点时 改变ray的t_max 每次计算交点的t 只有t_min < t < t_max才被判定为相交
### Texture
png格式即rgba8888纹理是位于gamma空间？
### SurfaceIntersection
根据相交点生成新的光线时 应该根据表面的法向量对光线的起点进行一定程度的偏移 不然相交点还是原先的旧点
### Light
#### AreaLight
##### DiffuseLight
由shape构成 均匀的发光 以后可能可以支持Ke ImageTexture
应该需要设计一个mesh area light 否则对于mesh代表的球形光源 area light数量太多
#### EnvironmentLight
环境光 由一个世界包围盒的外接球代表 有中心和半径两个参数
计算环境光的二维分布
注意pdf对于连续变量来说 其值是可以大于1的

## 渲染器
### PathTrace 路径追踪


### SPPM 光子映射
基本思路是两阶段 
第一阶段从相机发出光线 如果是specular的话 就继续发射 如果是diffuse的话 则停止并记录该点到所属的grid的链表中
第二阶段 从光源出发射光子 与场景相交 找到交点所属的grid 遍历链表其 找到那些半径包括该光子的可视点所属的像素 将该光子的贡献计算到这些像素
这里每一个grid元素存储的链表是那些SPPMPixel指针 即一个grid元素可以对应到多个像素 这当中的光子由可视点映射
一个像素每次发射光线 找到场景中的可视点 每一个可视点根据空间划分到一个grid当中 相当于把这个像素插入到这个grid的链表中
每一次迭代都会重新生成可视点 SPPMPixel中每一次迭代完之后只保留tau radius N direct_illum

### BDPT
默认的BSDF计算的PDF其实是Solid Angle 这个从积分式的微分项也可以看出
而在BDPT中根据多重采样的需要 光源路径涉及到多种采样策略 
需要将Solid Angle PDF转换为 PDF per unit area
Vertex中pdf_fwd存储的是从这个点到下一个点的pdf per unit area
pdf_bwd是从下一个点到这个点的pdf per unit area
accu_coef 的计算是根据solid angle pdf进行的
## Utility
atomic float add
## Source Code
### core
主要是定义一些抽象接口类
#### sampler
提供生成均匀分布的随机数 十分原始简单 效率可能比较低
#### material
##### glass
由fresnel确定折射和反射的百分比即概率
#### shape
几何形状的抽象基类 可以派生出各种具体的形状类
#### primitive
用于包装material和shape 以及可能的MediumInterface
#### aggregate

### scene
包含一个场景所需要的所有类型的变量 但是没有film
包括 场景的默认medium 相机 光源 环境光 primitive以及aggregate

### factory
用于提供创建具体派生类的函数接口

### utility

#### parallel.hpp
多线程加速渲染 每个线程负责一个tile(grid)

