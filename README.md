# AETHER ENGINE

A modular, open-source 3D game engine and world simulation platform.

> **Development status:** Early foundation stage. AETHER ENGINE is not production-ready. APIs, architecture details and supported platforms may change as the engine is developed.

## Project goals

AETHER ENGINE is being developed as an independent engine and simulation platform. Its long-term direction includes:

- A modular native runtime and desktop editor.
- A rendering hardware interface with Vulkan as the initial graphics backend.
- Scene, entity and asset pipelines.
- Physics and extensible scientific simulation systems.
- Procedural worlds, animation, audio and AI systems.
- Extensible gameplay scripting and networking.

These are project goals, not claims that the features are already implemented.

## Initial engineering baseline

- Language: C++20
- Build system: CMake
- Initial desktop targets: Windows and Linux
- Initial graphics API direction: Vulkan
- License: Apache-2.0

The M1 milestone focuses on the build and platform foundation, core runtime, memory and job infrastructure, ECS, scenes and continuous integration.

## Repository layout

The planned repository structure is documented in the project governance and architecture materials. Directories will be introduced as their corresponding subsystems enter implementation; this avoids presenting empty or unimplemented subsystems as completed features.

## Build

Requirements:

- CMake 3.25 or newer
- A compiler with C++20 support
- A supported native development environment

Configure and build the Debug preset:

```sh
cmake --preset debug
cmake --build --preset debug
```

Configure and build the Release preset:

```sh
cmake --preset release
cmake --build --preset release
```

The initial foundation target is intentionally minimal. Platform integration, runtime modules and automated tests will be added in their respective M1 work packages.

## Engineering principles

1. Keep engine-facing interfaces independent of third-party implementation types.
2. Prefer explicit ownership and predictable lifecycle behaviour.
3. Keep optional cloud services and AI providers outside the mandatory runtime.
4. Verify behaviour with reproducible tests and recorded evidence.
5. Track third-party dependencies, versions and licenses.
6. Do not claim a subsystem is implemented until its acceptance criteria have been verified.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Contributions must respect the project's architecture, licensing and review requirements.

## License

AETHER ENGINE is distributed under the Apache License 2.0. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Project status

The repository is being established. Consult the commit history and project documentation for verified implementation progress.
