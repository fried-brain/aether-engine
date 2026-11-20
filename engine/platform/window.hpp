#pragma once

#include <memory>
#include <string>

namespace aether::platform {

namespace detail {
struct PlatformState;
struct WindowImpl;
}

class Window final {
public:
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    ~Window();

    void close() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] std::uint32_t id() const noexcept;
    [[nodiscard]] std::string title() const;

private:
    friend class Platform;
    explicit Window(std::unique_ptr<detail::WindowImpl> impl) noexcept;

    std::unique_ptr<detail::WindowImpl> impl_;
};

} // namespace aether::platform
