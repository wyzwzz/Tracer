# Tracer
## 渲染器运行流程
    1.将一张film划分为多个tile，每一个tile由一个线程负责，tile在划分时不相交的，但是具体的tile在创建时会根据filter的半径进行校正
    2.每一个tile的渲染是单线程的，即依次渲染tile里的每一个pixel，每个tile创建一个arena，当一个pixel渲染完后reset arena

## 主要的一些接口类
### filter
用于image reconstruction 目前只实现高斯过滤
### camera

## 渲染器
### PathTrace 路径追踪

## Source Code
### core
主要是定义一些抽象接口类
#### sampler
提供生成均匀分布的随机数 十分原始简单 效率可能比较低
### factory
用于提供创建具体派生类的函数接口

### utility

#### parallel.hpp
多线程加速渲染 每个线程负责一个tile(grid)

