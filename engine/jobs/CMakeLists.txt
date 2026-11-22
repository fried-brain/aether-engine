add_library(aether_jobs STATIC src/job_system.cpp)
add_library(Aether::Jobs ALIAS aether_jobs)
target_compile_features(aether_jobs PUBLIC cxx_std_20)
target_include_directories(aether_jobs PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/engine>
)
target_link_libraries(aether_jobs PUBLIC Aether::Core Threads::Threads)

if(BUILD_TESTING)
    add_executable(aether_jobs_tests ${PROJECT_SOURCE_DIR}/tests/jobs/job_tests.cpp)
    target_link_libraries(aether_jobs_tests PRIVATE Aether::Jobs Threads::Threads)
    add_test(NAME aether.jobs.contracts COMMAND aether_jobs_tests)
endif()
