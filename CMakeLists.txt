cmake_minimum_required(VERSION 3.16)
project(Tracer LANGUAGES CXX C)

include(dep/glm.cmake)
include(dep/spdlog.cmake)
include(dep/tinyobjloader.cmake)
file(
        GLOB
        SRCS
        src/main.cpp
        src/core/*.hpp
        src/core/*.cpp
        src/renderer/*.hpp
        src/renderer/*.cpp
        src/utility/*.hpp
        src/utility/*.cpp
        src/factory/*.hpp
        src/factory/*.cpp
        src/filter/*.cpp
        src/camera/*.cpp
        src/shape/*cpp
        src/texture/*.cpp
        src/material/*.cpp
        src/light/*.cpp
        src/accelerator/*.cpp
        src/material/bxdf/*.cpp
)

add_executable(Tracer ${SRCS})

target_compile_features(Tracer PRIVATE cxx_std_17)

target_include_directories(
        Tracer
        PRIVATE
        src
        dep
)

target_link_libraries(
        Tracer
        PRIVATE
        glm::glm
        spdlog::spdlog
        tinyobjloader
)