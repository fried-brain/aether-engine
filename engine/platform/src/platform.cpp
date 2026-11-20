#include "aether/platform/platform.hpp"

#include "aether/platform/detail/platform_state.hpp"

#include <SDL3/SDL.h>

#include <memory>
#include <utility>

namespace aether::platform {
namespace {

[[nodiscard]] KeyCode to_key_code(SDL_Scancode scancode) noexcept {
    const auto value = static_cast<std::int32_t>(scancode);
    switch (value) {
        case 4: return KeyCode::A;
        case 5: return KeyCode::B;
        case 6: return KeyCode::C;
        case 7: return KeyCode::D;
        case 8: return KeyCode::E;
        case 9: return KeyCode::F;
        case 10: return KeyCode::G;
        case 11: return KeyCode::H;
        case 12: return KeyCode::I;
        case 13: return KeyCode::J;
        case 14: return KeyCode::K;
        case 15: return KeyCode::L;
        case 16: return KeyCode::M;
        case 17: return KeyCode::N;
        case 18: return KeyCode::O;
        case 19: return KeyCode::P;
        case 20: return KeyCode::Q;
        case 21: return KeyCode::R;
        case 22: return KeyCode::S;
        case 23: return KeyCode::T;
        case 24: return KeyCode::U;
        case 25: return KeyCode::V;
        case 26: return KeyCode::W;
        case 27: return KeyCode::X;
        case 28: return KeyCode::Y;
        case 29: return KeyCode::Z;
        case 30: return KeyCode::Digit1;
        case 31: return KeyCode::Digit2;
        case 32: return KeyCode::Digit3;
        case 33: return KeyCode::Digit4;
        case 34: return KeyCode::Digit5;
        case 35: return KeyCode::Digit6;
        case 36: return KeyCode::Digit7;
        case 37: return KeyCode::Digit8;
        case 38: return KeyCode::Digit9;
        case 39: return KeyCode::Digit0;
        case 40: return KeyCode::Enter;
        case 41: return KeyCode::Escape;
        case 42: return KeyCode::Backspace;
        case 43: return KeyCode::Tab;
        case 44: return KeyCode::Space;
        case 79: return KeyCode::Right;
        case 80: return KeyCode::Left;
        case 81: return KeyCode::Down;
        case 82: return KeyCode::Up;
        default: return KeyCode::Unknown;
    }
}

[[nodiscard]] InputEvent translate_event(const SDL_Event& event) noexcept {
    InputEvent translated{};
    switch (event.type) {
        case SDL_EVENT_QUIT:
            translated.type = InputEventType::QuitRequested;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            translated.type = InputEventType::WindowResized;
            translated.window_id = event.window.windowID;
            translated.x = static_cast<float>(event.window.data1);
            translated.y = static_cast<float>(event.window.data2);
            break;
        case SDL_EVENT_KEY_DOWN:
            translated.type = InputEventType::KeyPressed;
            translated.window_id = event.key.windowID;
            translated.key = to_key_code(event.key.scancode);
            translated.repeat = event.key.repeat;
            break;
        case SDL_EVENT_KEY_UP:
            translated.type = InputEventType::KeyReleased;
            translated.window_id = event.key.windowID;
            translated.key = to_key_code(event.key.scancode);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            translated.type = InputEventType::MouseButtonPressed;
            translated.window_id = event.button.windowID;
            translated.mouse_button = event.button.button;
            translated.x = event.button.x;
            translated.y = event.button.y;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            translated.type = InputEventType::MouseButtonReleased;
            translated.window_id = event.button.windowID;
            translated.mouse_button = event.button.button;
            translated.x = event.button.x;
            translated.y = event.button.y;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            translated.type = InputEventType::MouseMoved;
            translated.window_id = event.motion.windowID;
            translated.x = event.motion.x;
            translated.y = event.motion.y;
            translated.delta_x = event.motion.xrel;
            translated.delta_y = event.motion.yrel;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            translated.type = InputEventType::MouseWheel;
            translated.window_id = event.wheel.windowID;
            translated.delta_x = event.wheel.x;
            translated.delta_y = event.wheel.y;
            break;
        default:
            break;
    }
    return translated;
}

} // namespace

namespace detail {

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

} // namespace detail

bool Platform::initialize(std::string* error) noexcept {
    if (is_initialized()) {
        return true;
    }

    auto state = std::make_shared<detail::PlatformState>();
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        if (error != nullptr) {
            *error = SDL_GetError();
        }
        return false;
    }

    state->initialized = true;
    state_ = std::move(state);
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

void Platform::shutdown() noexcept {
    // Windows retain the shared platform state, so SDL remains initialized
    // until the final window has been destroyed.
    state_.reset();
}

bool Platform::is_initialized() const noexcept {
    return state_ && state_->initialized;
}

std::unique_ptr<Window> Platform::create_window(
    const WindowConfig& config,
    std::string* error
) {
    if (!is_initialized()) {
        if (error != nullptr) {
            *error = "Platform must be initialized before creating a window.";
        }
        return nullptr;
    }
    if (!config.is_valid()) {
        if (error != nullptr) {
            *error = "Window configuration requires a non-empty title and positive dimensions.";
        }
        return nullptr;
    }

    SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    if (config.hidden) {
        flags |= SDL_WINDOW_HIDDEN;
    }

    SDL_Window* native_window = SDL_CreateWindow(
        config.title.c_str(), config.width, config.height, flags
    );
    if (native_window == nullptr) {
        if (error != nullptr) {
            *error = SDL_GetError();
        }
        return nullptr;
    }

    auto impl = std::make_unique<detail::WindowImpl>(native_window, state_.get());
    // Keep the platform state alive for as long as this window exists.
    // The shared owner is stored by the public Window in the next implementation step.
    if (error != nullptr) {
        error->clear();
    }
    return std::unique_ptr<Window>{new Window{std::move(impl)}};
}

std::vector<InputEvent> Platform::poll_events() {
    std::vector<InputEvent> events;
    if (!is_initialized()) {
        return events;
    }

    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        const InputEvent translated = translate_event(event);
        if (translated.type != InputEventType::Unknown) {
            events.push_back(translated);
        }
    }
    return events;
}

} // namespace aether::platform
