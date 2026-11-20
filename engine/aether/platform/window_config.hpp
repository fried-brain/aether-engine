#pragma once

#include <string>

namespace aether::platform {

struct WindowConfig final {
    std::string title{"AETHER ENGINE"};
    int width{1280};
    int height{720};
    bool resizable{true};
    bool hidden{false};

    [[nodiscard]] bool is_valid() const noexcept {
        return !title.empty() && width > 0 && height > 0;
    }
};

} // namespace aether::platform
