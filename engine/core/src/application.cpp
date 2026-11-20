#include "aether/core/application.hpp"

#include <exception>
#include <string>
#include <utility>

namespace aether::core {
namespace {

Status callback_failure(ErrorCode code, std::string module_name,
                        const char* callback, const Status& cause) {
    std::string message = "Module '" + module_name + "' " + callback + " failed";
    if (!cause.message.empty()) message += ": " + cause.message;
    return Status::failure(code, std::move(message));
}

Status callback_exception(ErrorCode code, const std::string& module_name,
                          const char* callback) {
    try {
        throw;
    } catch (const std::exception& error) {
        return Status::failure(code, "Module '" + module_name + "' " + callback +
            " threw an exception: " + error.what());
    } catch (...) {
        return Status::failure(code, "Module '" + module_name + "' " + callback +
            " threw a non-standard exception.");
    }
}

void append_failure(Status& primary, const Status& additional) {
    if (additional.ok()) return;
    if (!primary.message.empty()) primary.message += " | ";
    primary.message += additional.message;
}

} // namespace

Application::Application(Logger& logger, ModuleRegistry modules)
    : logger_(logger), modules_(std::move(modules)) {}

Status Application::initialize() {
    if (state_ != ApplicationState::Created) {
        return Status::failure(ErrorCode::InvalidState,
            "Application can only be initialized from Created state.");
    }

    state_ = ApplicationState::Initializing;
    auto validation = modules_.validate();
    if (!validation) {
        failure_ = validation;
        state_ = ApplicationState::Failed;
        return failure_;
    }

    RuntimeContext context{logger_};
    for (auto* module : modules_.ordered_modules_) {
        Status result;
        try {
            result = module->on_initialize(context);
        } catch (...) {
            result = callback_exception(ErrorCode::ModuleInitializationFailed,
                std::string(module->name()), "on_initialize");
        }

        if (!result) {
            if (result.code != ErrorCode::ModuleInitializationFailed) {
                result = callback_failure(ErrorCode::ModuleInitializationFailed,
                    std::string(module->name()), "on_initialize", result);
            } else if (result.message.find(std::string(module->name())) == std::string::npos) {
                result.message = "Module '" + std::string(module->name()) +
                    "' on_initialize failed: " + result.message;
            }

            Status partial_cleanup;
            try {
                partial_cleanup = module->on_shutdown();
            } catch (...) {
                partial_cleanup = callback_exception(ErrorCode::ModuleShutdownFailed,
                    std::string(module->name()), "on_shutdown during initialization rollback");
            }
            if (!partial_cleanup) {
                append_failure(result, callback_failure(ErrorCode::ModuleShutdownFailed,
                    std::string(module->name()), "partial-initialization cleanup",
                    partial_cleanup));
            }

            const auto rollback = shutdown_initialized_modules();
            append_failure(result, rollback);
            failure_ = result;
            state_ = ApplicationState::Failed;
            return failure_;
        }

        initialized_modules_.push_back(module);
    }

    state_ = ApplicationState::Running;
    return Status::success();
}

Status Application::update(const FrameContext& frame) {
    if (state_ != ApplicationState::Running) {
        return Status::failure(ErrorCode::InvalidState,
            "Application update requires Running state.");
    }

    for (auto* module : initialized_modules_) {
        Status result;
        try {
            result = module->on_update(frame);
        } catch (...) {
            result = callback_exception(ErrorCode::ModuleUpdateFailed,
                std::string(module->name()), "on_update");
        }
        if (!result) {
            if (result.code != ErrorCode::ModuleUpdateFailed) {
                result = callback_failure(ErrorCode::ModuleUpdateFailed,
                    std::string(module->name()), "on_update", result);
            } else if (result.message.find(std::string(module->name())) == std::string::npos) {
                result.message = "Module '" + std::string(module->name()) +
                    " on_update failed: " + result.message;
            }
            const auto cleanup = shutdown_initialized_modules();
            append_failure(result, cleanup);
            failure_ = result;
            state_ = ApplicationState::Failed;
            return failure_;
        }
    }
    return Status::success();
}

Status Application::shutdown_initialized_modules() {
    Status aggregate = Status::success();
    for (auto it = initialized_modules_.rbegin(); it != initialized_modules_.rend(); ++it) {
        auto* module = *it;
        Status result;
        try {
            result = module->on_shutdown();
        } catch (...) {
            result = callback_exception(ErrorCode::ModuleShutdownFailed,
                std::string(module->name()), "on_shutdown");
        }
        if (!result) {
            auto failure = result.code == ErrorCode::ModuleShutdownFailed
                ? result
                : callback_failure(ErrorCode::ModuleShutdownFailed,
                    std::string(module->name()), "on_shutdown", result);
            if (aggregate.ok()) aggregate = std::move(failure);
            else append_failure(aggregate, failure);
        }
    }
    initialized_modules_.clear();
    return aggregate;
}

Status Application::shutdown() {
    if (state_ == ApplicationState::Stopped) return Status::success();
    if (state_ == ApplicationState::Failed) return failure_;
    if (state_ == ApplicationState::Created) {
        state_ = ApplicationState::Stopped;
        return Status::success();
    }
    if (state_ != ApplicationState::Running) {
        return Status::failure(ErrorCode::InvalidState,
            "Application shutdown is not valid in the current state.");
    }

    state_ = ApplicationState::Stopping;
    auto result = shutdown_initialized_modules();
    if (!result) {
        failure_ = result;
        state_ = ApplicationState::Failed;
        return failure_;
    }
    state_ = ApplicationState::Stopped;
    return Status::success();
}

ApplicationState Application::state() const noexcept { return state_; }

} // namespace aether::core
