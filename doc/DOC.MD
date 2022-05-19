# Tracer
## 渲染器运行流程
    1.将一张film划分为多个tile，每一个tile由一个线程负责，tile在划分时不相交的，但是具体的tile在创建时会根据filter的半径进行校正
    2.每一个tile的渲染是单线程的，即依次渲染tile里的每一个pixel，每个tile创建一个arena，当一个pixel渲染完后reset arena
    3.每一个像素的渲染，首先从Camera生成光线开始，光线的生成涉及到对像素在film上的采样和相机透镜位置上的采样，暂时不考虑时间
    4.生成光线后，接着是对场景的求交，找到交点处的几何信息和反射模型
## 主要的一些接口类
### filter
用于image reconstruction 目前只实现高斯过滤
### camera

### triangle
因为triangle一般以mesh的形式出现 会有十分多的triangle 所以多于triangle的存储需要特别设置
专门设置一个TriangleMesh类用于存放所有的mesh的顶点数据 在Triangle类中存放索引即可
但是这样子每一个Triangle对象会需要一个GeometricPrimitive包装 GeometricPrimitive虽然只需要存储指向材质和shape的ptr 但是也是不小的开销
另一种思路是 专门创建一个TriangleBVH的shape也是一种思路 这样子的话一个Triangle Mesh只需要一个GeometricPrimitive
## 渲染器
### PathTrace 路径追踪

## Source Code
### core
主要是定义一些抽象接口类
#### sampler
提供生成均匀分布的随机数 十分原始简单 效率可能比较低
#### material
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

