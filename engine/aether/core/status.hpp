#pragma once
#include <cstdint>
#include <string>
namespace aether::core {
enum class ErrorCode : std::uint8_t { None, InvalidState, InvalidArgument, DuplicateModule, MissingDependency, DependencyCycle, ModuleInitializationFailed, ModuleUpdateFailed, ModuleShutdownFailed, InternalError };
struct Status final {
    ErrorCode code{ErrorCode::None};
    std::string message;
    [[nodiscard]] bool ok() const noexcept;
    explicit operator bool() const noexcept;
    [[nodiscard]] static Status success();
    [[nodiscard]] static Status failure(ErrorCode code, std::string message);
};
} // namespace aether::core
