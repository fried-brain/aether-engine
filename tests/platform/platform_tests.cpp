#include "aether/platform/input_event.hpp"
#include "aether/platform/window_config.hpp"

#include <cstdlib>
#include <iostream>

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

} // namespace

int main() {
    test_default_window_configuration_is_valid();
    test_invalid_window_configurations_are_rejected();
    test_input_event_has_safe_defaults();
    std::cout << "AETHER platform contract tests passed.\n";
    return EXIT_SUCCESS;
}
