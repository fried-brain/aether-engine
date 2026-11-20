#pragma once

#include "aether/core/module_registry.hpp"

#include <vector>

namespace aether::core {

enum class ApplicationState : std::uint8_t {
    Created, Initializing, Running, Stopping, Stopped, Failed
};

class Application final {
public:
    Application(Logger& logger, ModuleRegistry modules);

    [[nodiscard]] Status initialize();
    [[nodiscard]] Status update(const FrameContext& frame);
    [[nodiscard]] Status shutdown();
    [[nodiscard]] ApplicationState state() const noexcept;

private:
    [[nodiscard]] Status shutdown_initialized_modules();

    Logger& logger_;
    ModuleRegistry modules_;
    std::vector<IModule*> initialized_modules_;
    ApplicationState state_{ApplicationState::Created};
    Status failure_{Status::success()};
};

} // namespace aether::core
