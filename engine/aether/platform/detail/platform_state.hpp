#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <utility>

namespace aether::platform::detail {

struct PlatformState final {
    PlatformState() = default;
    PlatformState(const PlatformState&) = delete;
    PlatformState& operator=(const PlatformState&) = delete;
    ~PlatformState();

    bool initialized{false};
};

struct WindowImpl final {
    WindowImpl(SDL_Window* native_window, std::shared_ptr<PlatformState> owner) noexcept
        : window(native_window), platform(std::move(owner)) {}

    WindowImpl(const WindowImpl&) = delete;
    WindowImpl& operator=(const WindowImpl&) = delete;
    ~WindowImpl();

    SDL_Window* window{nullptr};
    std::shared_ptr<PlatformState> platform;
};

} // namespace aether::platform::detail
