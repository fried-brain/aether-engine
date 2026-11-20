#pragma once

#include "aether/core/module.hpp"

#include <chrono>

namespace aether::core {

class Clock final {
public:
    Clock() noexcept;
    void reset() noexcept;
    [[nodiscard]] FrameContext tick() noexcept;

private:
    using ClockSource = std::chrono::steady_clock;
    ClockSource::time_point start_{};
    ClockSource::time_point previous_{};
    std::uint64_t frame_index_{0};
    bool has_ticked_{false};
};

} // namespace aether::core
