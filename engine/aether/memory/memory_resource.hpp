#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace aether::memory {

class IMemoryResource {
public:
    virtual ~IMemoryResource() = default;
    [[nodiscard]] virtual void* allocate(std::size_t size, std::size_t alignment) = 0;
    virtual void deallocate(void* pointer, std::size_t size,
                            std::size_t alignment) noexcept = 0;
};

class SystemMemoryResource final : public IMemoryResource {
public:
    [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment) override;
    void deallocate(void* pointer, std::size_t size,
                    std::size_t alignment) noexcept override;
};

struct MemoryStats final {
    std::uint64_t current_bytes{0};
    std::uint64_t peak_bytes{0};
    std::uint64_t total_allocations{0};
    std::uint64_t active_allocations{0};
    std::uint64_t failed_allocations{0};
    std::uint64_t invalid_deallocations{0};
    std::uint64_t size_mismatches{0};
};

class TrackingMemoryResource final : public IMemoryResource {
public:
    explicit TrackingMemoryResource(IMemoryResource& upstream) noexcept;
    ~TrackingMemoryResource() override;

    TrackingMemoryResource(const TrackingMemoryResource&) = delete;
    TrackingMemoryResource& operator=(const TrackingMemoryResource&) = delete;

    [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment) override;
    void deallocate(void* pointer, std::size_t size,
                    std::size_t alignment) noexcept override;
    [[nodiscard]] MemoryStats stats() const noexcept;

private:
    struct Allocation final {
        std::size_t size;
        std::size_t alignment;
    };

    IMemoryResource& upstream_;
    mutable std::mutex mutex_;
    std::unordered_map<void*, Allocation> allocations_;
    MemoryStats stats_{};
};

} // namespace aether::memory
