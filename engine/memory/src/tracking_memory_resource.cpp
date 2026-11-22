#include "aether/memory/memory_resource.hpp"

#include <new>
#include <utility>

namespace aether::memory {

TrackingMemoryResource::TrackingMemoryResource(IMemoryResource& upstream) noexcept
    : upstream_(upstream) {}

TrackingMemoryResource::~TrackingMemoryResource() {
    std::lock_guard lock(mutex_);
    for (const auto& [pointer, allocation] : allocations_) {
        upstream_.deallocate(pointer, allocation.size, allocation.alignment);
    }
    allocations_.clear();
    stats_.current_bytes = 0;
    stats_.active_allocations = 0;
}

void* TrackingMemoryResource::allocate(std::size_t size, std::size_t alignment) {
    void* pointer = nullptr;
    try {
        pointer = upstream_.allocate(size, alignment);
        {
            std::lock_guard lock(mutex_);
            const auto [entry, inserted] = allocations_.emplace(
                pointer, Allocation{size, alignment});
            (void)entry;
            if (!inserted) {
                upstream_.deallocate(pointer, size, alignment);
                ++stats_.failed_allocations;
                throw std::bad_alloc();
            }
            stats_.current_bytes += static_cast<std::uint64_t>(size);
            if (stats_.current_bytes > stats_.peak_bytes)
                stats_.peak_bytes = stats_.current_bytes;
            ++stats_.total_allocations;
            ++stats_.active_allocations;
        }
    } catch (...) {
        if (pointer != nullptr) {
            std::lock_guard lock(mutex_);
            const auto found = allocations_.find(pointer);
            if (found == allocations_.end()) {
                // Metadata insertion failed; the upstream allocation is still ours.
                ++stats_.failed_allocations;
                upstream_.deallocate(pointer, size, alignment);
            }
        }
        else {
            std::lock_guard lock(mutex_);
            ++stats_.failed_allocations;
        }
        throw;
    }
    return pointer;
}

void TrackingMemoryResource::deallocate(void* pointer, std::size_t size,
                                        std::size_t alignment) noexcept {
    if (pointer == nullptr) return;

    Allocation allocation{};
    {
        std::lock_guard lock(mutex_);
        const auto found = allocations_.find(pointer);
        if (found == allocations_.end()) {
            ++stats_.invalid_deallocations;
            return;
        }

        allocation = found->second;
        if (allocation.size != size || allocation.alignment != alignment)
            ++stats_.size_mismatches;
        stats_.current_bytes -= static_cast<std::uint64_t>(allocation.size);
        --stats_.active_allocations;
        allocations_.erase(found);
    }

    upstream_.deallocate(pointer, allocation.size, allocation.alignment);
}

MemoryStats TrackingMemoryResource::stats() const noexcept {
    std::lock_guard lock(mutex_);
    return stats_;
}

} // namespace aether::memory
