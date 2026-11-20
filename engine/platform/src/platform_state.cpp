#include "aether/platform/detail/platform_state.hpp"

namespace aether::platform::detail {

PlatformState::~PlatformState() {
    if (initialized) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        initialized = false;
    }
}

WindowImpl::~WindowImpl() {
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

} // namespace aether::platform::detail
