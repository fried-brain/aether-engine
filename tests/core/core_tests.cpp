#include "aether/core/logging.hpp"
#include "aether/core/status.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
namespace {
void expect(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\\n'; std::exit(EXIT_FAILURE); }
}
class RecordingSink final : public aether::core::ILogSink {
public:
    void write(aether::core::LogLevel level, std::string_view category, std::string_view message) override {
        const std::lock_guard lock(mutex);
        levels.push_back(level); categories.emplace_back(category); messages.emplace_back(message);
    }
    std::mutex mutex;
    std::vector<aether::core::LogLevel> levels;
    std::vector<std::string> categories, messages;
};
void test_status_success_and_failure() {
    using namespace aether::core;
    const auto success = Status::success();
    expect(success.ok() && static_cast<bool>(success), "success status should be truthy");
    const auto failure = Status::failure(ErrorCode::InvalidState, "bad state");
    expect(!failure.ok() && !static_cast<bool>(failure), "failure status should be false");
    expect(failure.code == ErrorCode::InvalidState, "failure code should be preserved");
    expect(failure.message == "bad state", "failure message should be preserved");
}
void test_logger_rejects_null_sink() {
    bool threw = false;
    try { aether::core::Logger logger(nullptr); }
    catch (const std::invalid_argument&) { threw = true; }
    expect(threw, "null sink should throw invalid_argument");
}
void test_logger_filters_levels_and_preserves_fields() {
    using namespace aether::core;
    auto sink = std::make_shared<RecordingSink>();
    Logger logger(sink);
    logger.set_level(LogLevel::Warning);
    logger.log(LogLevel::Info, "runtime", "filtered");
    logger.log(LogLevel::Warning, "runtime", "visible");
    expect(sink->messages.size() == 1, "below-threshold message should be filtered");
    expect(sink->levels[0] == LogLevel::Warning, "sink should receive level");
    expect(sink->categories[0] == "runtime", "sink should receive category");
    expect(sink->messages[0] == "visible", "sink should receive message");
    logger.set_level(LogLevel::Off);
    logger.log(LogLevel::Critical, "runtime", "also filtered");
    expect(sink->messages.size() == 1, "Off should filter every message");
}
void test_logger_concurrent_writes_are_complete() {
    using namespace aether::core;
    auto sink = std::make_shared<RecordingSink>();
    Logger logger(sink);
    logger.set_level(LogLevel::Trace);
    constexpr int thread_count = 8, messages_per_thread = 100;
    std::vector<std::thread> workers;
    for (int t = 0; t < thread_count; ++t) workers.emplace_back([&, t] {
        for (int i = 0; i < messages_per_thread; ++i)
            logger.log(LogLevel::Info, "worker", std::to_string(t) + ":" + std::to_string(i));
    });
    for (auto& worker : workers) worker.join();
    expect(sink->messages.size() == thread_count * messages_per_thread, "all concurrent records should arrive");
    for (const auto& message : sink->messages)
        expect(message.find(':') != std::string::npos, "each call should reach sink as one complete record");
}
}
int main() {
    test_status_success_and_failure();
    test_logger_rejects_null_sink();
    test_logger_filters_levels_and_preserves_fields();
    test_logger_concurrent_writes_are_complete();
    std::cout << "AETHER core contract tests passed.\\n";
    return EXIT_SUCCESS;
}
