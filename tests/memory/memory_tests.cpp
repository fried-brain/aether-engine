#include "aether/memory/memory_resource.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_system_resource_alignment_and_zero_size() {
    aether::memory::SystemMemoryResource resource;
    void* aligned = resource.allocate(73, 64);
    expect(reinterpret_cast<std::uintptr_t>(aligned) % 64 == 0,
           "system resource must honor requested alignment");
    resource.deallocate(aligned, 73, 64);
    void* zero = resource.allocate(0, alignof(std::max_align_t));
    expect(zero != nullptr, "zero-size allocation must return releasable storage");
    resource.deallocate(zero, 0, alignof(std::max_align_t));
}

void test_tracking_resource_statistics_and_mismatch_recovery() {
    aether::memory::SystemMemoryResource upstream;
    aether::memory::TrackingMemoryResource resource(upstream);
    void* first = resource.allocate(128, 32);
    void* second = resource.allocate(64, 16);
    auto stats = resource.stats();
    expect(stats.current_bytes == 192, "current bytes must include active allocations");
    expect(stats.peak_bytes == 192, "peak bytes must track maximum live bytes");
    expect(stats.active_allocations == 2, "active allocation count must be correct");
    expect(stats.total_allocations == 2, "total allocation count must be correct");

    resource.deallocate(first, 1, 8);
    stats = resource.stats();
    expect(stats.current_bytes == 64, "mismatched deallocation must use recorded size");
    expect(stats.size_mismatches == 1, "mismatched size or alignment must be recorded");
    resource.deallocate(second, 64, 16);
    stats = resource.stats();
    expect(stats.current_bytes == 0 && stats.active_allocations == 0,
           "all live allocation statistics must return to zero");
}

void test_tracking_resource_counts_failed_upstream_allocations() {
    class FailingResource final : public aether::memory::IMemoryResource {
    public:
        void* allocate(std::size_t, std::size_t) override { throw std::bad_alloc(); }
        void deallocate(void*, std::size_t, std::size_t) noexcept override {}
    } upstream;
    aether::memory::TrackingMemoryResource resource(upstream);
    bool threw = false;
    try { (void)resource.allocate(32, 16); }
    catch (const std::bad_alloc&) { threw = true; }
    expect(threw, "upstream allocation failure must propagate");
    expect(resource.stats().failed_allocations == 1,
           "upstream allocation failure must be counted");
}

void test_tracking_resource_rejects_invalid_pointer_without_forwarding() {
    aether::memory::SystemMemoryResource upstream;
    aether::memory::TrackingMemoryResource resource(upstream);
    int stack_value = 0;
    resource.deallocate(&stack_value, sizeof(stack_value), alignof(int));
    const auto stats = resource.stats();
    expect(stats.invalid_deallocations == 1,
           "unknown pointer must be counted and not forwarded upstream");
    expect(stats.current_bytes == 0, "invalid deallocation must not alter live bytes");
}

void test_tracking_resource_concurrent_accounting() {
    aether::memory::SystemMemoryResource upstream;
    aether::memory::TrackingMemoryResource resource(upstream);
    constexpr int workers = 8;
    constexpr int iterations = 100;
    std::vector<std::thread> threads;
    for (int worker = 0; worker < workers; ++worker) {
        threads.emplace_back([&] {
            for (int i = 0; i < iterations; ++i) {
                void* memory = resource.allocate(24, 16);
                resource.deallocate(memory, 24, 16);
            }
        });
    }
    for (auto& thread : threads) thread.join();
    const auto stats = resource.stats();
    expect(stats.total_allocations == workers * iterations,
           "concurrent allocation count must be exact");
    expect(stats.current_bytes == 0 && stats.active_allocations == 0,
           "concurrent allocations must all be released");
}
}

int main() {
    test_system_resource_alignment_and_zero_size();
    test_tracking_resource_statistics_and_mismatch_recovery();
    test_tracking_resource_counts_failed_upstream_allocations();
    test_tracking_resource_rejects_invalid_pointer_without_forwarding();
    test_tracking_resource_concurrent_accounting();
    std::cout << "AETHER memory contract tests passed.\n";
    return EXIT_SUCCESS;
}
