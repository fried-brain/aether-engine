#pragma once

#include "aether/core/logging.hpp"
#include "aether/core/status.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace aether::core {

using ModuleId = std::string;

struct RuntimeContext final {
    Logger& logger;
};

struct FrameContext final {
    double delta_seconds{0.0};
    double elapsed_seconds{0.0};
    std::uint64_t frame_index{0};
};

class IModule {
public:
    virtual ~IModule() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::span<const ModuleId> dependencies() const noexcept = 0;
    virtual Status on_initialize(RuntimeContext& context) = 0;
    virtual Status on_update(const FrameContext& frame) = 0;
    virtual Status on_shutdown() = 0;
};

} // namespace aether::core
