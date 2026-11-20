# Third-Party Notices and Dependency Register

AETHER ENGINE tracks third-party software separately from its own Apache-2.0-licensed source.

## Integrated dependencies

| Component | Version | Intended use | License | Source |
|---|---:|---|---|---|
| SDL3 | 3.4.20 | Native windowing and input/event platform layer | zlib | https://github.com/libsdl-org/SDL/tree/release-3.4.20 |

SDL3 is fetched by CMake at the pinned `release-3.4.20` tag and built as a static dependency for the initial desktop platform target. SDL3's own license and notices remain applicable.

## Evaluated candidates — not integrated

| Candidate | Intended use | Integration status |
|---|---|---|
| EnTT | ECS implementation behind AETHER interfaces | Not integrated |
| Jolt Physics | Rigid-body physics foundation | Not integrated |
| Lua | Gameplay scripting | Not integrated |
| spdlog | Logging | Not integrated |
| Catch2 v3 | Testing | Not integrated |
| GLM | Mathematics | Not integrated |
| fmt | Formatting, if required | Not integrated |

Each additional dependency must have its exact version, source, license, attribution requirements, maintenance status and security review recorded before integration.

The AETHER ENGINE Apache-2.0 license does not replace or modify the license of any third-party component.
