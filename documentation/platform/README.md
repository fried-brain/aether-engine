# Native Platform Layer

The M1.1 platform layer provides AETHER-owned interfaces for native window
lifecycle and input events. SDL3 is an implementation dependency, not part of
the public engine-facing API.

## Public headers

- `aether/platform/platform.hpp` — initialize/shutdown, create windows, poll events.
- `aether/platform/window.hpp` — move-only RAII window handle.
- `aether/platform/window_config.hpp` — validated window configuration.
- `aether/platform/input_event.hpp` — engine-owned input event model.

## SDL dependency

CMake fetches the pinned SDL3 `release-3.4.20` tag when no suitable package is
provided by the build. The platform target links against `SDL3::SDL3`.

## Lifetime

A `Platform` must be initialized before it creates a `Window`. Windows are
move-only and destroy their native SDL window when closed or destroyed.
Destroy windows before shutting down the platform. The implementation keeps
the SDL subsystem alive while windows retain platform state.

## Current scope

M1.1 currently covers window configuration, window creation/destruction,
event polling and translation for quit, resize, keyboard and mouse events.
It does not yet provide a renderer, controller/gamepad abstraction, clipboard,
IME/text input, raw input or multi-window policy.
