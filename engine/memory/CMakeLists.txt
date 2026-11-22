add_library(aether_memory STATIC
    src/system_memory_resource.cpp
    src/tracking_memory_resource.cpp
)
add_library(Aether::Memory ALIAS aether_memory)
target_compile_features(aether_memory PUBLIC cxx_std_20)
target_include_directories(aether_memory PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/engine>
)

if(BUILD_TESTING)
    add_executable(aether_memory_tests ${PROJECT_SOURCE_DIR}/tests/memory/memory_tests.cpp)
    target_link_libraries(aether_memory_tests PRIVATE Aether::Memory Threads::Threads)
    add_test(NAME aether.memory.contracts COMMAND aether_memory_tests)
endif()
