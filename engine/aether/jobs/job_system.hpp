#pragma once

#include "aether/core/status.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace aether::jobs {

class JobSystem;

namespace detail {
struct JobState;
}

enum class JobSystemState {
    Created,
    Running,
    Stopping,
    Stopped
};

struct JobSystemConfig final {
    std::size_t worker_count{0};
};

class JobHandle final {
public:
    JobHandle() = default;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] aether::core::Status wait() const;

private:
    friend class JobSystem;
    explicit JobHandle(std::shared_ptr<detail::JobState> state) noexcept;
    std::shared_ptr<detail::JobState> state_;
};

struct JobSubmission final {
    aether::core::Status status;
    JobHandle handle;
};

class JobSystem final {
public:
    explicit JobSystem(JobSystemConfig config = {});
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    [[nodiscard]] aether::core::Status start();
    [[nodiscard]] JobSubmission submit(std::function<void()> function);
    [[nodiscard]] aether::core::Status shutdown();
    [[nodiscard]] JobSystemState state() const noexcept;

private:
    void worker_loop() noexcept;

    JobSystemConfig config_;
    mutable std::mutex mutex_;
    std::mutex lifecycle_mutex_;
    std::condition_variable condition_;
    std::queue<std::shared_ptr<detail::JobState>> queue_;
    std::vector<std::thread> workers_;
    JobSystemState state_{JobSystemState::Created};
};

} // namespace aether::jobs
