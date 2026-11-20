# Contributing to AETHER ENGINE

AETHER ENGINE is in its early foundation stage. Contributions should be focused, reviewable and consistent with the approved architecture.

## Before contributing

1. Read the project README and relevant architecture documentation.
2. Check existing issues and pull requests before starting substantial work.
3. For a new subsystem or behaviour change, open a discussion or issue first.
4. Do not introduce a third-party dependency without documenting its version, license, purpose and maintenance status.
5. Do not submit proprietary code, unlicensed assets, credentials or confidential material.

## Changes

- Keep changes focused and reasonably small.
- Follow the existing C++20 and CMake conventions.
- Prefer clear ownership and explicit error handling.
- Keep third-party implementation types behind AETHER-facing interfaces where required by the architecture.
- Add or update tests when behaviour changes.
- Update documentation when interfaces or build instructions change.
- Record material architectural decisions.

## Pull requests

A pull request should explain:

- What changed.
- Why the change is needed.
- Which M1 requirement or issue it addresses.
- How it was tested.
- Any limitations or follow-up work.

Do not claim a feature is complete unless its acceptance criteria have been verified.

## Licensing

By intentionally submitting a contribution for inclusion, you agree that it is offered under the project's Apache License 2.0 terms, subject to any separate written agreement. You retain ownership of your contribution.

All third-party code and assets must retain their applicable licenses and notices.
