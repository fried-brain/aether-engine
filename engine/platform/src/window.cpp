#include "aether/platform/window.hpp"

#include "aether/platform/detail/platform_state.hpp"

#include <utility>

namespace aether::platform {

Window::Window(std::unique_ptr<detail::WindowImpl> impl) noexcept
    : impl_(std::move(impl)) {}

Window::Window(Window&&) noexcept = default;
Window& Window::operator=(Window&&) noexcept = default;
Window::~Window() = default;

void Window::close() noexcept {
    if (impl_) {
        if (impl_->window != nullptr) {
            SDL_DestroyWindow(impl_->window);
            impl_->window = nullptr;
        }
        impl_->platform.reset();
    }
}

bool Window::is_open() const noexcept {
    return impl_ && impl_->window != nullptr;
}

int Window::width() const noexcept {
    if (!is_open()) {
        return 0;
    }
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(impl_->window, &width, &height);
    return width;
}

int Window::height() const noexcept {
    if (!is_open()) {
        return 0;
    }
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(impl_->window, &width, &height);
    return height;
}

std::uint32_t Window::id() const noexcept {
    return is_open() ? SDL_GetWindowID(impl_->window) : 0U;
}

std::string Window::title() const {
    return is_open() ? std::string{SDL_GetWindowTitle(impl_->window)} : std::string{};
}

} // namespace aether::platform
