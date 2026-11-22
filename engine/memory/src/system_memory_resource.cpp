#include "aether/memory/memory_resource.hpp"

#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>

namespace aether::memory {
namespace {

std::size_t effective_alignment(std::size_t alignment) {
    if (alignment == 0 || !std::has_single_bit(alignment)) {
        throw std::invalid_argument("Memory alignment must be a non-zero power of two.");
    }
    return std::max(alignment, alignof(std::max_align_t));
}

} // namespace

void* SystemMemoryResource::allocate(std::size_t size, std::size_t alignment) {
    const auto actual_alignment = effective_alignment(alignment);
    return ::operator new(size == 0 ? 1 : size,
                          static_cast<std::align_val_t>(actual_alignment));
}

void SystemMemoryResource::deallocate(void* pointer, std::size_t,
                                     std::size_t alignment) noexcept {
    if (pointer == nullptr) return;
    const auto actual_alignment = alignment == 0
        ? alignof(std::max_align_t)
        : std::max(alignment, alignof(std::max_align_t));
    ::operator delete(pointer, static_cast<std::align_val_t>(actual_alignment));
}

} // namespace aether::memory
