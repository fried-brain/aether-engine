# AETHER Memory Resources

The M1.3 memory subsystem exposes engine-owned allocation interfaces under `aether::memory`.

## Resource ownership

Implementations of `IMemoryResource` allocate and release storage using the same resource instance. The resource must outlive every allocation made through it.

`SystemMemoryResource` uses C++ aligned allocation and accepts non-zero power-of-two alignments. Zero-size requests return a non-null block that can be deallocated through the same resource.

`TrackingMemoryResource` wraps an upstream resource and records active pointers, sizes and alignments. It provides a synchronized `MemoryStats` snapshot. Unknown pointers are counted and ignored rather than forwarded upstream. A size or alignment mismatch is counted, and the recorded allocation metadata is used for safe release.

Tracking metadata uses the standard allocator rather than the tracked upstream resource, avoiding recursive bookkeeping. The tracking resource releases any remaining tracked allocations during destruction; callers should nevertheless release their own allocations before destroying the resource.

## Statistics

- `current_bytes`: bytes currently tracked as live.
- `peak_bytes`: highest observed live-byte total.
- `total_allocations`: successful tracked allocations.
- `active_allocations`: currently tracked blocks.
- `failed_allocations`: upstream or tracking-registration allocation failures.
- `invalid_deallocations`: unknown non-null pointers passed to deallocate.
- `size_mismatches`: deallocations whose supplied size or alignment differs from recorded metadata.

The tracking resource supports concurrent allocation, deallocation and statistics snapshots. Its upstream resource must also support the concurrency pattern in which it is used.
