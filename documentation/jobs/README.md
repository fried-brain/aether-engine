# AETHER Job System

The M1.3 job system is a small C++20 worker pool exposed as `Aether::Jobs`.

## Starting and submitting work

Create a `JobSystem`, call `start()`, and submit callable work with `submit()`. A successful submission returns a valid `JobHandle`. Calling `wait()` on the handle blocks until completion and returns the job result.

A worker count of zero selects `std::thread::hardware_concurrency()`, falling back to one worker when the platform reports zero. An explicit positive count is used as supplied.

## Queue and failure contract

The queue is an unbounded, thread-safe FIFO. Every accepted job is executed exactly once. Exceptions are captured in the associated handle as `JobExecutionFailed`; they do not terminate the worker.

A worker must not block waiting for unfinished work submitted to the same job system. Such a wait returns `InvalidState` rather than risking deadlock. Waiting on already-completed work remains valid.

## Shutdown and lifetime

Shutdown atomically stops accepting new work, drains all accepted jobs, joins workers and enters `Stopped`. Repeated shutdown is successful. A job system cannot be restarted after stopping.

Do not call `shutdown()` or destroy the job system from one of its own worker callbacks. The owning scope must keep the job system alive until all submitted work has completed and shutdown has returned.

The job system owns worker threads and must not be copied or moved. Job handles may be copied and shared among waiters.
