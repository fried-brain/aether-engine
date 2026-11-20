#include "aether/core/logging.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
namespace aether::core {
namespace {
const char* level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    case LogLevel::Off: return "OFF";
    }
    return "UNKNOWN";
}
std::tm utc_time(std::time_t value) {
    std::tm result{};
#if defined(_WIN32)
    gmtime_s(&result, &value);
#else
    gmtime_r(&value, &result);
#endif
    return result;
}
} // namespace
void ConsoleLogSink::write(LogLevel level, std::string_view category, std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto tm = utc_time(time);
    std::ostringstream record;
    record << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << '.' << std::setw(3)
           << std::setfill('0') << milliseconds.count() << "Z [" << level_name(level)
           << "] [" << category << "] " << message;
    const std::lock_guard lock(mutex_);
    std::cerr << record.str() << '\\n';
}
Logger::Logger(std::shared_ptr<ILogSink> sink) : sink_(std::move(sink)) {
    if (!sink_) throw std::invalid_argument("Logger requires a non-null log sink.");
}
void Logger::set_level(LogLevel level) noexcept {
    const std::lock_guard lock(mutex_);
    level_ = level;
}
LogLevel Logger::level() const noexcept {
    const std::lock_guard lock(mutex_);
    return level_;
}
void Logger::log(LogLevel level, std::string_view category, std::string_view message) {
    const std::lock_guard lock(mutex_);
    if (level_ == LogLevel::Off || level < level_) return;
    sink_->write(level, category, message);
}
} // namespace aether::core
