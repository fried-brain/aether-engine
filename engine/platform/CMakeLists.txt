add_library(aether_platform STATIC
    src/platform.cpp
    src/platform_state.cpp
    src/window.cpp
)

add_library(Aether::Platform ALIAS aether_platform)

target_compile_features(aether_platform PUBLIC cxx_std_20)

target_include_directories(aether_platform
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/engine>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aether_platform PRIVATE SDL3::SDL3)

if(BUILD_TESTING)
    add_executable(aether_platform_tests
        ${PROJECT_SOURCE_DIR}/tests/platform/platform_tests.cpp
    )
    target_link_libraries(aether_platform_tests PRIVATE Aether::Platform SDL3::SDL3)
    add_test(NAME aether.platform.contracts COMMAND aether_platform_tests)
    set_tests_properties(aether.platform.contracts PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy"
    )
endif()
