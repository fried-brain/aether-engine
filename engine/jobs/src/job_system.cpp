#include "aether/jobs/job_system.hpp"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace aether::jobs {
namespace detail {

struct JobState final {
    explicit JobState(const JobSystem* system) : owner(system) {}
    const JobSystem* owner;
    std::function<void()> function;
    std::mutex mutex;
    std::condition_variable condition;
    bool complete{false};
    aether::core::Status result{aether::core::Status::success()};
};

} // namespace detail
namespace {

thread_local const JobSystem* current_worker_system = nullptr;

aether::core::Status job_exception_status() {
    try {
        throw;
    } catch (const std::exception& error) {
        return aether::core::Status::failure(
            aether::core::ErrorCode::JobExecutionFailed,
            std::string("Job threw an exception: ") + error.what());
    } catch (...) {
        return aether::core::Status::failure(
            aether::core::ErrorCode::JobExecutionFailed,
            "Job threw a non-standard exception.");
    }
}

} // namespace

JobHandle::JobHandle(std::shared_ptr<detail::JobState> state) noexcept
    : state_(std::move(state)) {}

bool JobHandle::valid() const noexcept {
    return static_cast<bool>(state_);
}

aether::core::Status JobHandle::wait() const {
    if (!state_) {
        return aether::core::Status::failure(
            aether::core::ErrorCode::InvalidState, "Cannot wait on an empty job handle.");
    }

    std::unique_lock lock(state_->mutex);
    if (current_worker_system == state_->owner && !state_->complete) {
        return aether::core::Status::failure(
            aether::core::ErrorCode::InvalidState,
            "A worker cannot block waiting for unfinished work from the same job system.");
    }
    state_->condition.wait(lock, [this] { return state_->complete; });
    return state_->result;
}

JobSystem::JobSystem(JobSystemConfig config) : config_(config) {}

JobSystem::~JobSystem() {
    (void)shutdown();
}

aether::core::Status JobSystem::start() {
    std::lock_guard lifecycle_lock(lifecycle_mutex_);
    {
        std::lock_guard lock(mutex_);
        if (state_ != JobSystemState::Created) {
            return aether::core::Status::failure(
                aether::core::ErrorCode::InvalidState,
                "JobSystem can only start from Created state.");
        }
        state_ = JobSystemState::Running;
    }

    auto count = config_.worker_count;
    if (count == 0) {
        count = static_cast<std::size_t>(std::thread::hardware_concurrency());
        if (count == 0) count = 1;
    }

    try {
        workers_.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    } catch (const std::exception& error) {
        {
            std::lock_guard lock(mutex_);
            state_ = JobSystemState::Stopping;
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
        workers_.clear();
        {
            std::lock_guard lock(mutex_);
            state_ = JobSystemState::Stopped;
        }
        return aether::core::Status::failure(
            aether::core::ErrorCode::InternalError,
            std::string("Failed to start job workers: ") + error.what());
    } catch (...) {
        {
            std::lock_guard lock(mutex_);
            state_ = JobSystemState::Stopping;
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
        workers_.clear();
        {
            std::lock_guard lock(mutex_);
            state_ = JobSystemState::Stopped;
        }
        return aether::core::Status::failure(
            aether::core::ErrorCode::InternalError,
            "Failed to start job workers due to a non-standard exception.");
    }

    return aether::core::Status::success();
}

JobSubmission JobSystem::submit(std::function<void()> function) {
    if (!function) {
        return {aether::core::Status::failure(
            aether::core::ErrorCode::InvalidArgument,
            "Cannot submit an empty job."), {}};
    }

    std::shared_ptr<detail::JobState> job;
    try {
        job = std::make_shared<detail::JobState>(this);
        job->function = std::move(function);
        {
            std::lock_guard lock(mutex_);
            if (state_ != JobSystemState::Running) {
                return {aether::core::Status::failure(
                    aether::core::ErrorCode::InvalidState,
                    "JobSystem is not accepting work."), {}};
            }
            queue_.push(job);
        }
    } catch (const std::exception& error) {
        return {aether::core::Status::failure(
            aether::core::ErrorCode::InternalError,
            std::string("Failed to queue job: ") + error.what()), {}};
    } catch (...) {
        return {aether::core::Status::failure(
            aether::core::ErrorCode::InternalError,
            "Failed to queue job due to a non-standard exception."), {}};
    }

    condition_.notify_one();
    return {aether::core::Status::success(), JobHandle(std::move(job))};
}

aether::core::Status JobSystem::shutdown() {
    if (current_worker_system == this) {
        return aether::core::Status::failure(
            aether::core::ErrorCode::InvalidState,
            "JobSystem shutdown cannot be called from one of its worker callbacks.");
    }

    std::lock_guard lifecycle_lock(lifecycle_mutex_);
    {
        std::lock_guard lock(mutex_);
        if (state_ == JobSystemState::Stopped) return aether::core::Status::success();
        if (state_ == JobSystemState::Created) {
            state_ = JobSystemState::Stopped;
            return aether::core::Status::success();
        }
        if (state_ == JobSystemState::Running) state_ = JobSystemState::Stopping;
    }

    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    workers_.clear();
    {
        std::lock_guard lock(mutex_);
        state_ = JobSystemState::Stopped;
    }
    return aether::core::Status::success();
}

JobSystemState JobSystem::state() const noexcept {
    std::lock_guard lock(mutex_);
    return state_;
}


void JobSystem::worker_loop() noexcept {
    const auto* previous_system = current_worker_system;
    current_worker_system = this;

    for (;;) {
        std::shared_ptr<detail::JobState> job;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] {
                return state_ != JobSystemState::Running || !queue_.empty();
            });
            if (queue_.empty()) {
                if (state_ == JobSystemState::Stopping || state_ == JobSystemState::Stopped)
                    break;
                continue;
            }
            job = std::move(queue_.front());
            queue_.pop();
        }

        aether::core::Status result = aether::core::Status::success();
        try {
            job->function();
        } catch (...) {
            result = job_exception_status();
        }

        {
            std::lock_guard lock(job->mutex);
            job->result = std::move(result);
            job->complete = true;
        }
        job->condition.notify_all();
    }

    current_worker_system = previous_system;
}

} // namespace aether::jobs
