#include "aether/platform/platform.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_default_window_configuration_is_valid() {
    const aether::platform::WindowConfig config{};
    expect(config.is_valid(), "default window configuration should be valid");
    expect(config.width == 1280, "default width should be 1280");
    expect(config.height == 720, "default height should be 720");
    expect(config.resizable, "default window should be resizable");
    expect(!config.hidden, "default window should be visible");
}

void test_invalid_window_configurations_are_rejected() {
    auto config = aether::platform::WindowConfig{};
    config.width = 0;
    expect(!config.is_valid(), "zero width should be rejected");

    config = {};
    config.height = -1;
    expect(!config.is_valid(), "negative height should be rejected");

    config = {};
    config.title.clear();
    expect(!config.is_valid(), "empty title should be rejected");
}

void test_input_event_has_safe_defaults() {
    const aether::platform::InputEvent event{};
    expect(event.type == aether::platform::InputEventType::Unknown,
           "default event type should be Unknown");
    expect(event.key == aether::platform::KeyCode::Unknown,
           "default key should be Unknown");
    expect(event.window_id == 0, "default window id should be zero");
    expect(!event.repeat, "default key repeat should be false");
}

void test_platform_requires_initialization_before_window_creation() {
    aether::platform::Platform platform;
    std::string error;
    const auto window = platform.create_window({}, &error);
    expect(!window, "window creation before initialization should fail");
    expect(!error.empty(), "window creation failure should explain the reason");
    expect(platform.poll_events().empty(),
           "polling events before initialization should return an empty list");
}

void test_window_lifecycle_and_platform_state_ownership() {
    aether::platform::Platform platform;
    std::string error;
    expect(platform.initialize(&error), "platform initialization should succeed");
    expect(platform.is_initialized(), "platform should report initialized");

    aether::platform::WindowConfig config;
    config.title = "AETHER platform test";
    config.width = 640;
    config.height = 480;
    config.hidden = true;

    auto window = platform.create_window(config, &error);
    expect(window != nullptr, "platform should create a hidden test window");
    expect(window->is_open(), "new window should be open");
    expect(window->width() == 640, "window width should match configuration");
    expect(window->height() == 480, "window height should match configuration");
    expect(window->title() == config.title, "window title should match configuration");
    expect(window->id() != 0, "created window should have a valid ID");

    platform.shutdown();
    expect(!platform.is_initialized(), "platform should report shutdown");
    expect(window->is_open(),
           "window should retain the SDL subsystem after platform shutdown");

    window->close();
    expect(!window->is_open(), "closed window should no longer be open");
    expect(window->width() == 0, "closed window width should be zero");
    expect(window->height() == 0, "closed window height should be zero");
}

void test_quit_and_keyboard_events_are_translated() {
    aether::platform::Platform platform;
    std::string error;
    expect(platform.initialize(&error), "platform initialization should succeed");

    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    expect(SDL_PushEvent(&quit), "quit event should enter SDL event queue");

    SDL_Event close{};
    close.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    close.window.windowID = 77;
    expect(SDL_PushEvent(&close), "window close event should enter SDL event queue");

    SDL_Event key{};
    key.type = SDL_EVENT_KEY_DOWN;
    key.key.scancode = SDL_SCANCODE_A;
    key.key.repeat = true;
    expect(SDL_PushEvent(&key), "keyboard event should enter SDL event queue");

    const auto events = platform.poll_events();
    bool saw_quit = false;
    bool saw_close = false;
    bool saw_key = false;
    for (const auto& event : events) {
        if (event.type == aether::platform::InputEventType::QuitRequested) {
            saw_quit = true;
        }
        if (event.type == aether::platform::InputEventType::WindowCloseRequested) {
            saw_close = event.window_id == 77;
        }
        if (event.type == aether::platform::InputEventType::KeyPressed) {
            saw_key = event.key == aether::platform::KeyCode::A && event.repeat;
        }
    }

    expect(saw_quit, "SDL quit event should translate to AETHER quit event");
    expect(saw_close, "SDL window close request should retain its window ID");
    expect(saw_key, "SDL keyboard event should translate to AETHER key event");
    platform.shutdown();
}

} // namespace

int main() {
    test_default_window_configuration_is_valid();
    test_invalid_window_configurations_are_rejected();
    test_input_event_has_safe_defaults();
    test_platform_requires_initialization_before_window_creation();
    test_window_lifecycle_and_platform_state_ownership();
    test_quit_and_keyboard_events_are_translated();
    std::cout << "AETHER platform contract tests passed.\n";
    return EXIT_SUCCESS;
}
