#include "aether/core/application.hpp"
#include "aether/core/clock.hpp"
#include "aether/core/event_dispatcher.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace aether::core;

void expect_runtime(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class RecordingSink final : public ILogSink {
public:
    void write(LogLevel, std::string_view, std::string_view) override {}
};

class TestModule final : public IModule {
public:
    TestModule(std::string module_name, std::vector<ModuleId> module_dependencies,
               std::vector<std::string>& event_log)
        : name_(std::move(module_name)), dependencies_(std::move(module_dependencies)),
          events_(event_log) {}

    std::string_view name() const noexcept override { return name_; }
    std::span<const ModuleId> dependencies() const noexcept override { return dependencies_; }

    Status on_initialize(RuntimeContext&) override {
        events_.push_back("init:" + name_);
        if (throw_initialize) throw std::runtime_error("initialize boom");
        if (throw_nonstandard_initialize) throw 7;
        return initialize_status;
    }

    Status on_update(const FrameContext& frame) override {
        events_.push_back("update:" + name_);
        received_frame = frame;
        if (throw_update) throw std::runtime_error("update boom");
        return update_status;
    }

    Status on_shutdown() override {
        events_.push_back("shutdown:" + name_);
        if (throw_shutdown) throw std::runtime_error("shutdown boom");
        return shutdown_status;
    }

    std::string name_;
    std::vector<ModuleId> dependencies_;
    std::vector<std::string>& events_;
    Status initialize_status{Status::success()};
    Status update_status{Status::success()};
    Status shutdown_status{Status::success()};
    FrameContext received_frame{};
    bool throw_initialize{false};
    bool throw_nonstandard_initialize{false};
    bool throw_update{false};
    bool throw_shutdown{false};
};

TestModule* add_module(ModuleRegistry& registry,
    const std::string& name, std::vector<ModuleId> dependencies,
    std::vector<std::string>& events) {
    auto module = std::make_unique<TestModule>(name, std::move(dependencies), events);
    auto* view = module.get();
    const auto result = registry.add(std::move(module));
    expect_runtime(result.ok(), "test module should register");
    return view;
}

void test_registry_rejects_duplicate_and_null_modules() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    const auto null_result = registry.add(nullptr);
    expect_runtime(null_result.code == ErrorCode::InvalidArgument, "null module must be rejected");
    add_module(registry, "renderer", {}, events);
    auto duplicate = std::make_unique<TestModule>("renderer", std::vector<ModuleId>{}, events);
    const auto duplicate_result = registry.add(std::move(duplicate));
    expect_runtime(duplicate_result.code == ErrorCode::DuplicateModule,
                   "duplicate module names must be rejected");
}

void test_missing_dependency_fails_before_initialization() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    add_module(registry, "world", {"physics"}, events);
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    const auto result = app.initialize();
    expect_runtime(result.code == ErrorCode::MissingDependency, "missing dependency must fail validation");
    expect_runtime(events.empty(), "validation failure must occur before callbacks");
    expect_runtime(app.state() == ApplicationState::Failed, "validation failure must fail application");
}

void test_dependency_cycle_fails_before_initialization() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    add_module(registry, "alpha", {"beta"}, events);
    add_module(registry, "beta", {"alpha"}, events);
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    const auto result = app.initialize();
    expect_runtime(result.code == ErrorCode::DependencyCycle, "dependency cycle must be rejected");
    expect_runtime(events.empty(), "cycle validation must happen before callbacks");
}

void test_dependency_order_is_deterministic() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    add_module(registry, "C", {"A"}, events);
    add_module(registry, "A", {}, events);
    add_module(registry, "B", {}, events);
    add_module(registry, "D", {"B"}, events);
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    expect_runtime(app.initialize().ok(), "valid registry should initialize");
    const std::vector<std::string> expected{"init:A", "init:C", "init:B", "init:D"};
    expect_runtime(events == expected, "topological order must be deterministic and stable by registration order");
    expect_runtime(app.shutdown().ok(), "ordered modules should shut down");
}

void test_initialization_failure_cleans_failing_module_then_prior_modules() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    add_module(registry, "A", {}, events);
    auto failing = add_module(registry, "B", {}, events);
    add_module(registry, "C", {}, events);
    failing->initialize_status = Status::failure(ErrorCode::InvalidArgument, "deliberate failure");
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    const auto result = app.initialize();
    const std::vector<std::string> expected{
        "init:A", "init:B", "shutdown:B", "shutdown:A"
    };
    expect_runtime(result.code == ErrorCode::ModuleInitializationFailed,
                   "initialization failure should use ModuleInitializationFailed");
    expect_runtime(events == expected, "failed module cleanup must precede reverse rollback");
    expect_runtime(result.message.find("deliberate failure") != std::string::npos,
                   "original initialization failure must remain in diagnostic");
    expect_runtime(app.state() == ApplicationState::Failed, "initialization failure should fail app");
    expect_runtime(app.shutdown().message == result.message,
                   "shutdown after failure must return stored failure without repeating callbacks");
    expect_runtime(events == expected, "failed shutdown call must not repeat cleanup");
}

void test_initialization_exception_runs_partial_cleanup() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    auto failing = add_module(registry, "throws", {}, events);
    failing->throw_nonstandard_initialize = true;
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    const auto result = app.initialize();
    expect_runtime(result.code == ErrorCode::ModuleInitializationFailed,
                   "non-standard callback exception must become initialization failure");
    expect_runtime(result.message.find("non-standard") != std::string::npos,
                   "non-standard exception should be identified");
    const std::vector<std::string> expected{"init:throws", "shutdown:throws"};
    expect_runtime(events == expected, "failing module must receive one partial cleanup callback");
}

void test_update_receives_frame_and_fails_fast_with_reverse_cleanup() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    add_module(registry, "A", {}, events);
    auto failing = add_module(registry, "B", {}, events);
    add_module(registry, "C", {}, events);
    failing->update_status = Status::failure(ErrorCode::InvalidArgument, "update fault");
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    expect_runtime(app.initialize().ok(), "application should initialize");
    events.clear();
    const FrameContext frame{0.125, 4.5, 17};
    const auto result = app.update(frame);
    const std::vector<std::string> expected{
        "update:A", "update:B", "shutdown:C", "shutdown:B", "shutdown:A"
    };
    expect_runtime(result.code == ErrorCode::ModuleUpdateFailed, "update error should be categorized");
    expect_runtime(events == expected, "update should fail fast and clean all modules in reverse order");
    expect_runtime(result.message.find("update fault") != std::string::npos,
                   "update failure must remain primary");
    expect_runtime(failing->received_frame.delta_seconds == frame.delta_seconds &&
                   failing->received_frame.elapsed_seconds == frame.elapsed_seconds &&
                   failing->received_frame.frame_index == frame.frame_index,
                   "module must receive the exact frame context");
    expect_runtime(app.state() == ApplicationState::Failed, "update failure should fail app");
}


void test_update_exception_is_converted_and_rolls_back() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    add_module(registry, "A", {}, events);
    auto failing = add_module(registry, "B", {}, events);
    failing->throw_update = true;
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    expect_runtime(app.initialize().ok(), "application should initialize");
    events.clear();
    const auto result = app.update(FrameContext{});
    const std::vector<std::string> expected{
        "update:A", "update:B", "shutdown:B", "shutdown:A"
    };
    expect_runtime(result.code == ErrorCode::ModuleUpdateFailed,
                   "update exception must become ModuleUpdateFailed");
    expect_runtime(result.message.find("update boom") != std::string::npos,
                   "update exception detail must be preserved");
    expect_runtime(events == expected, "update exception must trigger reverse-order rollback");
}

void test_partial_initialization_cleanup_failure_is_appended() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    auto failing = add_module(registry, "partial", {}, events);
    failing->initialize_status = Status::failure(ErrorCode::InvalidArgument, "primary init fault");
    failing->shutdown_status = Status::failure(ErrorCode::InternalError, "partial cleanup fault");
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    const auto result = app.initialize();
    expect_runtime(result.code == ErrorCode::ModuleInitializationFailed,
                   "initialization failure must remain the primary error category");
    expect_runtime(result.message.find("primary init fault") != std::string::npos &&
                   result.message.find("partial cleanup fault") != std::string::npos,
                   "partial cleanup failure must be appended to the initialization failure");
    expect_runtime(events == std::vector<std::string>{"init:partial", "shutdown:partial"},
                   "failing module partial cleanup must run exactly once");
}

void test_shutdown_attempts_every_module_and_aggregates_failures() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    auto a = add_module(registry, "A", {}, events);
    auto b = add_module(registry, "B", {}, events);
    a->shutdown_status = Status::failure(ErrorCode::InternalError, "A cleanup fault");
    b->throw_shutdown = true;
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    expect_runtime(app.initialize().ok(), "application should initialize");
    events.clear();
    const auto result = app.shutdown();
    const std::vector<std::string> expected{"shutdown:B", "shutdown:A"};
    expect_runtime(events == expected, "shutdown must continue after a module cleanup failure");
    expect_runtime(result.code == ErrorCode::ModuleShutdownFailed, "shutdown failure should be categorized");
    expect_runtime(result.message.find("shutdown boom") != std::string::npos &&
                   result.message.find("A cleanup fault") != std::string::npos,
                   "aggregate diagnostic must include every cleanup failure");
    expect_runtime(app.state() == ApplicationState::Failed, "failed cleanup should enter Failed state");
    const auto repeated = app.shutdown();
    expect_runtime(repeated.message == result.message, "repeated shutdown must return stored failure");
    expect_runtime(events == expected, "repeated shutdown must not repeat callbacks");
}

void test_application_rejects_illegal_transitions_and_stops_idempotently() {
    std::vector<std::string> events;
    ModuleRegistry registry;
    Logger logger(std::make_shared<RecordingSink>());
    Application app(logger, std::move(registry));
    expect_runtime(app.update({}).code == ErrorCode::InvalidState,
                   "update before initialize must be rejected");
    expect_runtime(app.initialize().ok(), "empty application should initialize");
    expect_runtime(app.initialize().code == ErrorCode::InvalidState,
                   "repeated initialize must be rejected");
    expect_runtime(app.shutdown().ok(), "running application should stop");
    expect_runtime(app.state() == ApplicationState::Stopped, "successful shutdown should stop app");
    expect_runtime(app.shutdown().ok(), "shutdown after Stopped must be idempotent");
}

void test_clock_first_tick_reset_and_monotonic_progress() {
    Clock clock;
    const auto first = clock.tick();
    expect_runtime(first.delta_seconds == 0.0 && first.elapsed_seconds == 0.0 &&
                   first.frame_index == 0, "first tick after reset must be zero baseline");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const auto second = clock.tick();
    expect_runtime(second.delta_seconds >= 0.0 && second.elapsed_seconds >= 0.0,
                   "clock values must never be negative");
    expect_runtime(second.frame_index == 1, "frame index must increment after baseline tick");
    clock.reset();
    const auto reset_tick = clock.tick();
    expect_runtime(reset_tick.delta_seconds == 0.0 && reset_tick.elapsed_seconds == 0.0 &&
                   reset_tick.frame_index == 0, "reset must restore initial clock state");
}

struct NumberEvent final { int value; };

void test_event_dispatch_order_unsubscribe_and_stable_snapshot() {
    EventDispatcher dispatcher;
    std::vector<int> calls;
    Subscription second;
    bool added = false;
    dispatcher.subscribe<NumberEvent>([&](const NumberEvent& event) {
        calls.push_back(event.value * 10 + 1);
        dispatcher.unsubscribe(second);
        if (!added) {
            added = true;
            dispatcher.subscribe<NumberEvent>([&](const NumberEvent& next) {
                calls.push_back(next.value * 10 + 3);
            });
        }
    });
    second = dispatcher.subscribe<NumberEvent>([&](const NumberEvent& event) {
        calls.push_back(event.value * 10 + 2);
    });

    expect_runtime(dispatcher.dispatch(NumberEvent{1}).ok(), "first dispatch should succeed");
    expect_runtime(calls == std::vector<int>{11},
                   "removed handlers are skipped and new handlers wait until next dispatch");
    calls.clear();
    expect_runtime(dispatcher.dispatch(NumberEvent{2}).ok(), "second dispatch should succeed");
    expect_runtime(calls == std::vector<int>{21, 23},
                   "remaining handlers should run in subscription order");
}

void test_event_handler_exception_does_not_stop_later_handlers() {
    EventDispatcher dispatcher;
    int later_handler_calls = 0;
    dispatcher.subscribe<NumberEvent>([](const NumberEvent&) {
        throw std::runtime_error("handler fault");
    });
    dispatcher.subscribe<NumberEvent>([&](const NumberEvent&) { ++later_handler_calls; });
    const auto result = dispatcher.dispatch(NumberEvent{5});
    expect_runtime(result.code == ErrorCode::InternalError,
                   "handler exceptions should be reported as InternalError");
    expect_runtime(result.message.find("1") != std::string::npos,
                   "dispatch failure should report failed handler count");
    expect_runtime(later_handler_calls == 1,
                   "later handlers must run after an earlier handler throws");
}

} // namespace

void run_runtime_tests() {
    test_registry_rejects_duplicate_and_null_modules();
    test_missing_dependency_fails_before_initialization();
    test_dependency_cycle_fails_before_initialization();
    test_dependency_order_is_deterministic();
    test_initialization_failure_cleans_failing_module_then_prior_modules();
    test_initialization_exception_runs_partial_cleanup();
    test_update_receives_frame_and_fails_fast_with_reverse_cleanup();
    test_update_exception_is_converted_and_rolls_back();
    test_partial_initialization_cleanup_failure_is_appended();
    test_shutdown_attempts_every_module_and_aggregates_failures();
    test_application_rejects_illegal_transitions_and_stops_idempotently();
    test_clock_first_tick_reset_and_monotonic_progress();
    test_event_dispatch_order_unsubscribe_and_stable_snapshot();
    test_event_handler_exception_does_not_stop_later_handlers();
}
