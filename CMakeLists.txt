cmake_minimum_required(VERSION 3.25)

project(AetherEngine
    VERSION 0.1.0
    DESCRIPTION "A modular, open-source 3D game engine and world simulation platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckCXXSourceCompiles)

check_cxx_source_compiles([=[
    #include <concepts>
    #include <span>
    #include <type_traits>

    template <typename T>
    concept Integral = std::is_integral_v<T>;

    static_assert(Integral<int>);

    int main() {
        int values[] = {1, 2, 3};
        std::span<int> view{values};
        return view.size() == 3 ? 0 : 1;
    }
]=] AETHER_COMPILER_HAS_REQUIRED_CXX20)

if(NOT AETHER_COMPILER_HAS_REQUIRED_CXX20)
    message(FATAL_ERROR "The selected compiler does not provide the required C++20 language/library features.")
endif()

add_library(aether_engine INTERFACE)

target_compile_features(aether_engine INTERFACE cxx_std_20)

target_include_directories(aether_engine
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
)

message(STATUS "AETHER ENGINE ${PROJECT_VERSION}")
message(STATUS "C++ standard requirement: C++20")
message(STATUS "C++20 compiler capability check: passed")
message(STATUS "This initial target is an interface-only foundation; compiled engine modules will be added in subsequent M1 tasks.")
