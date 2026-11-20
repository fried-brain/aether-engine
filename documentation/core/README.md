# AETHER Core Runtime

The M1.2 core runtime is a native C++20 library exposed as `Aether::Core`. It is designed to run without SDL, a window, a GPU, a network connection or external services.

## Module lifecycle

Implement `aether::core::IModule` and register each module in a `ModuleRegistry` using a unique, non-empty name. Dependencies refer to other registered module names.

The registry validates every dependency before any module callback runs. It rejects missing dependencies and cycles, then computes a deterministic topological order. When multiple modules are ready, registration order breaks the tie.

`Application` takes ownership of a registry and borrows a `Logger`. Its normal lifecycle is:

```text
Created -> Initializing -> Running -> Stopping -> Stopped
```

Initialization, update or shutdown failures move the application to `Failed`. Module callbacks are exception boundaries: standard and non-standard exceptions are converted to failed `Status` values.

If initialization fails, the failing module receives one best-effort `on_shutdown()` call for partial-resource cleanup, followed by reverse-order cleanup of modules that initialized successfully. An update failure stops further updates and cleans up all initialized modules in reverse order. Shutdown attempts every initialized module even if an earlier cleanup fails; the first failure is retained and later failures are appended to its diagnostic message.

A failed application stores its primary failure. Calling `shutdown()` again after failure returns that stored result without repeating cleanup callbacks. A successfully stopped application treats repeated shutdown as successful and idempotent.

## Frame time

`Clock` uses `std::chrono::steady_clock`. The first `tick()` after construction or `reset()` returns a zero delta, zero elapsed time and frame index zero. Later ticks report non-negative seconds and increment the frame index. The clock does not sleep, throttle or clamp frame time.

## Events

`EventDispatcher` delivers ordinary C++ event value types synchronously on the calling thread. Handlers run in subscription order.

Dispatch uses a stable subscription snapshot: handlers added during a dispatch do not receive the in-progress event, and handlers unsubscribed before their turn are skipped. If a handler throws, the dispatcher continues with remaining handlers and returns an `InternalError` status containing the number of failed handlers.

The dispatcher is single-thread confined in M1.2. Concurrent subscription, unsubscription and dispatch are unsupported; callers must provide external confinement.

## Logging and concurrency

A `Logger` serializes its own calls into its sink and protects threshold reads/writes. A sink shared by multiple Logger instances must provide its own synchronization. `ConsoleLogSink` does so internally and writes UTC ISO-8601 millisecond records to standard error.

## Testing

The core contract tests are part of the normal CTest suite. They are intended to run in both Debug and Release on Windows and Linux and do not require a display server or GPU.
