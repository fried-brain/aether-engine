#include "aether/core/clock.hpp"

namespace aether::core {

Clock::Clock() noexcept { reset(); }

void Clock::reset() noexcept {
    start_ = ClockSource::now();
    previous_ = start_;
    frame_index_ = 0;
    has_ticked_ = false;
}

FrameContext Clock::tick() noexcept {
    const auto now = ClockSource::now();
    if (!has_ticked_) {
        previous_ = now;
        has_ticked_ = true;
        return {};
    }

    const auto delta = std::chrono::duration<double>(now - previous_).count();
    const auto elapsed = std::chrono::duration<double>(now - start_).count();
    previous_ = now;
    ++frame_index_;
    return FrameContext{delta < 0.0 ? 0.0 : delta,
                        elapsed < 0.0 ? 0.0 : elapsed,
                        frame_index_};
}

} // namespace aether::core
