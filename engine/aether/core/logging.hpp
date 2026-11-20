#pragma once
#include <memory>
#include <mutex>
#include <string_view>
namespace aether::core {
enum class LogLevel : unsigned char { Trace, Debug, Info, Warning, Error, Critical, Off };
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(LogLevel level, std::string_view category, std::string_view message) = 0;
};
class ConsoleLogSink final : public ILogSink {
public:
    void write(LogLevel level, std::string_view category, std::string_view message) override;
private:
    std::mutex mutex_;
};
class Logger final {
public:
    // Throws std::invalid_argument when sink is null.
    explicit Logger(std::shared_ptr<ILogSink> sink);
    void set_level(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;
    void log(LogLevel level, std::string_view category, std::string_view message);
private:
    std::shared_ptr<ILogSink> sink_;
    mutable std::mutex mutex_;
    LogLevel level_{LogLevel::Info};
};
} // namespace aether::core
