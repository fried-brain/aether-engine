#include "aether/core/status.hpp"
#include <utility>
namespace aether::core {
bool Status::ok() const noexcept { return code == ErrorCode::None; }
Status::operator bool() const noexcept { return ok(); }
Status Status::success() { return {}; }
Status Status::failure(ErrorCode error_code, std::string error_message) {
    return {error_code, std::move(error_message)};
}
} // namespace aether::core
