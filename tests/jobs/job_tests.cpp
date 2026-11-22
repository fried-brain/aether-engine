#include "aether/jobs/job_system.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_jobs_execute_and_handles_complete() {
    aether::jobs::JobSystem system({2});
    expect(system.start().ok(), "job system should start");
    std::atomic<int> value{0};
    auto submitted = system.submit([&] { value.fetch_add(1); });
    expect(submitted.status.ok() && submitted.handle.valid(),
           "running system should accept a job");
    expect(submitted.handle.wait().ok(), "job handle should report successful completion");
    expect(value.load() == 1, "accepted job should execute exactly once");
    expect(system.shutdown().ok(), "job system should shut down");
}

void test_job_exception_is_reported_and_worker_survives() {
    aether::jobs::JobSystem system({1});
    expect(system.start().ok(), "job system should start");
    auto failed = system.submit([] { throw std::runtime_error("job failure"); });
    expect(failed.status.ok(), "throwing job should be accepted");
    const auto failure = failed.handle.wait();
    expect(failure.code == aether::core::ErrorCode::JobExecutionFailed,
           "job exception should become JobExecutionFailed");
    std::atomic<bool> later_job_ran{false};
    auto later = system.submit([&] { later_job_ran.store(true); });
    expect(later.status.ok() && later.handle.wait().ok(),
           "worker should continue after a failed job");
    expect(later_job_ran.load(), "later job should execute");
    expect(system.shutdown().ok(), "system should shut down after job failure");
}

void test_shutdown_drains_accepted_jobs_and_rejects_new_work() {
    aether::jobs::JobSystem system({3});
    expect(system.start().ok(), "job system should start");
    std::atomic<int> completed{0};
    std::vector<aether::jobs::JobHandle> handles;
    for (int i = 0; i < 80; ++i) {
        auto submission = system.submit([&] { completed.fetch_add(1); });
        expect(submission.status.ok(), "job should be accepted before shutdown");
        handles.push_back(submission.handle);
    }
    expect(system.shutdown().ok(), "shutdown should drain accepted jobs");
    expect(completed.load() == 80, "all accepted jobs must finish before shutdown returns");
    for (const auto& handle : handles)
        expect(handle.wait().ok(), "drained job handles should report success");
    auto rejected = system.submit([] {});
    expect(!rejected.status.ok() && !rejected.handle.valid(),
           "submission after shutdown must be rejected");
    expect(system.shutdown().ok(), "repeated shutdown should be idempotent");
}

void test_handles_support_multiple_waiters_and_empty_submission_is_rejected() {
    aether::jobs::JobSystem system({2});
    expect(system.start().ok(), "job system should start");
    std::atomic<bool> release{false};
    auto submitted = system.submit([&] {
        while (!release.load()) std::this_thread::yield();
    });
    expect(submitted.status.ok(), "job should be accepted");
    std::atomic<int> successful_waiters{0};
    std::thread first([&] { if (submitted.handle.wait().ok()) ++successful_waiters; });
    std::thread second([&] { if (submitted.handle.wait().ok()) ++successful_waiters; });
    release.store(true);
    first.join();
    second.join();
    expect(successful_waiters.load() == 2, "multiple waiters must observe the same completion");
    auto empty = system.submit({});
    expect(empty.status.code == aether::core::ErrorCode::InvalidArgument &&
           !empty.handle.valid(), "empty callable must be rejected");
    expect(system.shutdown().ok(), "system should shut down");
}

void test_worker_cannot_deadlock_waiting_on_its_own_system() {
    aether::jobs::JobSystem system({1});
    expect(system.start().ok(), "job system should start");
    std::atomic<aether::core::ErrorCode> observed{aether::core::ErrorCode::None};
    auto submitted = system.submit([&] {
        auto nested = system.submit([] {});
        observed.store(nested.handle.wait().code);
    });
    expect(submitted.status.ok() && submitted.handle.wait().ok(),
           "outer job should complete");
    expect(observed.load() == aether::core::ErrorCode::InvalidState,
           "worker waiting on unfinished same-system work must be rejected");
    expect(system.shutdown().ok(), "system should shut down");
}
}

int main() {
    test_jobs_execute_and_handles_complete();
    test_job_exception_is_reported_and_worker_survives();
    test_shutdown_drains_accepted_jobs_and_rejects_new_work();
    test_handles_support_multiple_waiters_and_empty_submission_is_rejected();
    test_worker_cannot_deadlock_waiting_on_its_own_system();
    std::cout << "AETHER job contract tests passed.\n";
    return EXIT_SUCCESS;
}
