#pragma once

#include <cstdint>

namespace aether::platform {

enum class InputEventType : std::uint8_t {
    Unknown,
    QuitRequested,
    WindowResized,
    WindowCloseRequested,
    KeyPressed,
    KeyReleased,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseWheel
};

// Physical keyboard scancode values use the USB HID keyboard usage IDs.
// This keeps the public API independent of SDL headers and keycode types.
enum class KeyCode : std::int32_t {
    Unknown = 0,
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Digit1 = 30, Digit2, Digit3, Digit4, Digit5,
    Digit6, Digit7, Digit8, Digit9, Digit0,
    Enter = 40,
    Escape = 41,
    Backspace = 42,
    Tab = 43,
    Space = 44,
    Right = 79,
    Left = 80,
    Down = 81,
    Up = 82
};

struct InputEvent final {
    InputEventType type{InputEventType::Unknown};
    std::uint32_t window_id{0};
    KeyCode key{KeyCode::Unknown};
    std::uint8_t mouse_button{0};
    float x{0.0F};
    float y{0.0F};
    float delta_x{0.0F};
    float delta_y{0.0F};
    bool repeat{false};
};

} // namespace aether::platform
