cmake_minimum_required(VERSION 3.25)

project(AetherEngine
    VERSION 0.1.0
    DESCRIPTION "A modular, open-source 3D game engine and world simulation platform"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckCXXSourceCompiles)
include(CTest)
include(FetchContent)

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

# SDL is used only behind AETHER-owned platform interfaces.
# Pin a stable release instead of following SDL's moving development branch.
set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "Disable SDL install rules" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "Disable SDL audio for the platform foundation milestone" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "Disable SDL joystick support for the platform foundation milestone" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "Disable SDL haptics for the platform foundation milestone" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "Disable SDL HIDAPI for the platform foundation milestone" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "Disable SDL sensors for the platform foundation milestone" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "Disable SDL camera for the platform foundation milestone" FORCE)
set(SDL_RENDER OFF CACHE BOOL "Disable SDL renderer; AETHER will own its graphics layer" FORCE)
set(SDL_WAYLAND OFF CACHE BOOL "Use X11 on Linux CI for the initial platform backend" FORCE)
set(SDL_KMSDRM OFF CACHE BOOL "Disable KMSDRM for the initial desktop platform backend" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.20
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3)

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3 did not provide the expected SDL3::SDL3 CMake target.")
endif()

add_library(aether_engine INTERFACE)
target_compile_features(aether_engine INTERFACE cxx_std_20)
target_include_directories(aether_engine
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/engine>
)

add_subdirectory(engine/platform)
add_subdirectory(engine/core)
add_subdirectory(engine/memory)
add_subdirectory(engine/jobs)

message(STATUS "AETHER ENGINE ${PROJECT_VERSION}")
message(STATUS "C++ standard requirement: C++20")
message(STATUS "C++20 compiler capability check: passed")
message(STATUS "SDL3 version: 3.4.20 (pinned release tag)")
