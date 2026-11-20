#pragma once

#include "aether/platform/input_event.hpp"
#include "aether/platform/window.hpp"
#include "aether/platform/window_config.hpp"

#include <memory>
#include <string>
#include <vector>

namespace aether::platform {

class Platform final {
public:
    Platform() = default;
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    Platform(Platform&&) noexcept = default;
    Platform& operator=(Platform&&) noexcept = default;
    ~Platform() = default;

    [[nodiscard]] bool initialize(std::string* error = nullptr) noexcept;
    void shutdown() noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;

    [[nodiscard]] std::unique_ptr<Window> create_window(
        const WindowConfig& config,
        std::string* error = nullptr
    );

    [[nodiscard]] std::vector<InputEvent> poll_events();

private:
    std::shared_ptr<detail::PlatformState> state_;
};

} // namespace aether::platform
