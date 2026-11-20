cmake_minimum_required(VERSION 3.25)

project(AetherEngine
    VERSION 0.1.0
    DESCRIPTION "A modular, open-source 3D game engine and world simulation platform"
    LANGUAGES CXX
)

add_library(aether_engine INTERFACE)

target_compile_features(aether_engine INTERFACE cxx_std_20)

target_include_directories(aether_engine
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
)

message(STATUS "AETHER ENGINE ${PROJECT_VERSION}")
message(STATUS "C++ standard requirement: C++20")
message(STATUS "This initial target is an interface-only foundation; compiled engine modules will be added in subsequent M1 tasks.")
