#pragma once

#include <SDL3/SDL.h>

namespace aether::platform::detail {

struct PlatformState final {
    PlatformState() = default;
    PlatformState(const PlatformState&) = delete;
    PlatformState& operator=(const PlatformState&) = delete;
    ~PlatformState();

    bool initialized{false};
};

struct WindowImpl final {
    WindowImpl(SDL_Window* native_window, PlatformState* owner) noexcept
        : window(native_window), platform(owner) {}

    WindowImpl(const WindowImpl&) = delete;
    WindowImpl& operator=(const WindowImpl&) = delete;
    ~WindowImpl();

    SDL_Window* window{nullptr};
    PlatformState* platform{nullptr};
};

} // namespace aether::platform::detail
