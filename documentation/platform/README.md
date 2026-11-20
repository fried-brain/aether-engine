# Native Platform Layer

The M1.1 platform layer provides AETHER-owned interfaces for native window lifecycle and input events. SDL3 is an implementation dependency and is intentionally excluded from public AETHER headers.

## Public headers

- `aether/platform/platform.hpp` — initialize/shutdown, create windows, poll events.
- `aether/platform/window.hpp` — move-only RAII window handle.
- `aether/platform/window_config.hpp` — validated window configuration.
- `aether/platform/input_event.hpp` — engine-owned input event model.

## SDL dependency

CMake fetches the pinned SDL3 `release-3.4.20` tag and builds the static library. SDL3 is linked privately by the platform library. Its types do not appear in the public platform headers.

## Lifetime and ownership

A `Platform` must be initialized before it creates a `Window`. Windows are move-only and destroy their native SDL window when closed or destroyed.

A window retains shared ownership of the SDL platform state. Calling `Platform::shutdown()` prevents further platform operations through that object, but does not tear down SDL while a previously created window still exists. The SDL subsystem is shut down after the final window releases the shared state.

## Events currently translated

- Application quit
- Window resize
- Window close request
- Keyboard press/release (physical USB HID scancode values for the documented key subset)
- Mouse button press/release
- Mouse movement
- Mouse wheel

Unknown SDL events are ignored rather than leaking SDL event types through the public API.

## Tests

The platform contract test executable checks configuration validation, pre-initialization errors, hidden-window lifecycle, shared platform-state lifetime, and quit/keyboard event translation. CI uses SDL's dummy video driver to run window lifecycle tests without requiring a physical display.

## Current limitations

M1.1 does not yet provide rendering/surface creation, controller/gamepad abstraction, clipboard, IME/text input, raw input, drag-and-drop, multi-window policy or platform capability reporting.
