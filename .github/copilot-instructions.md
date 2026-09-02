# Mir Copilot Instructions

Mir is a Wayland compositor library written in C++26 (with some Rust components). It provides a
stable ABI layer (MirAL) for building shells, plus graphics/input hardware abstraction and window
management.

This file holds repository-wide essentials. **Detailed, area-specific guidance lives in
[`.github/instructions/`](instructions/)** and is applied automatically when you touch matching
files:

| File                               | Applies to                                                                      |
| ---------------------------------- | ------------------------------------------------------------------------------- |
| `cpp.instructions.md`              | `**/*.h`, `**/*.cpp` — C++ style, common patterns, logging                      |
| `abi-symbols.instructions.md`      | `symbols.map`, library `CMakeLists.txt`, debian symbols — ABI/symbol management |
| `wayland-protocol.instructions.md` | `src/server/frontend_wayland/**`, `src/wayland/**`, `wayland-protocols/**`      |
| `testing.instructions.md`          | `tests/**` — gtest/gmock, flaky tests, WLCS                                     |
| `rust.instructions.md`             | `src/platforms/evdev-rs/**` — Rust/CXX safety                                   |
| `cmake.instructions.md`            | `**/CMakeLists.txt`, `cmake/**`                                                 |
| `docs.instructions.md`             | `doc/**` — Sphinx docs                                                          |
| `live-config.instructions.md`      | MirAL live-config headers/sources                                               |

## Architecture

### Core layers

- **include/**: Public API headers with minimal implementation details. Interfaces here should not expose platform-specific types.
- **src/**: Implementation organized by component. Internal headers stay within their component directory.
- **miral/** (Mir Abstraction Layer): ABI-stable layer in `include/miral/miral/` providing shell-building APIs like `MirRunner`, `WindowManagementPolicy`, window managers, and accessibility features.
- **Platform backends**: `src/platforms/{gbm-kms,atomic-kms,x11,wayland,virtual}` - hardware abstraction layers for different graphics/input systems.
- **Input platforms**: `evdev` (C++), `evdev-rs` (Rust with CXX bindings)

### Key components

- **Server** (`src/server/`): Core compositor services (scene, compositor, graphics, frontend_xwayland)
- **Renderers** (`src/renderers/gl/`): GL-based rendering implementations
- **Tests** (`tests/`): Unit, integration, and performance tests using gtest/gmock
- **Documentation** (`doc/sphinx/`): Sphinx-based docs using the Canonical theme, hosted on Read the Docs and proxied via `canonical.com/mir/docs/`

## Building & testing

Mir uses CMake. A basic build:

```sh
cmake -B build
cmake --build build -j$(nproc)
```

For faster iteration, use ccache and the mold linker:

```sh
sudo apt install ccache mold
cmake -B build -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DMIR_USE_PRECOMPILED_HEADERS=OFF -DMIR_USE_LD=mold
cmake --build build -j$(nproc)
```

Run tests from the build directory:

```sh
cd build
ctest --output-on-failure                                    # Run all tests
bin/mir_unit_tests --gtest_filter='TestSuite.test_name'      # A specific test
bin/miral-test --gtest_filter='*WindowManager*'
```

Use the smallest targeted test, build, or lint command that covers your change; escalate to the
full suite only when targeted validation shows it's needed.

## CI

CI is driven by [spread](https://github.com/snapcore/spread) (`spread.yaml`) via
`.github/workflows/spread.yml`. The matrix covers multiple distros (Ubuntu, Debian, Fedora,
Alpine), architectures (amd64, arm64, armhf), sanitizers (ASan/TSan/UBSan, GCC and Clang), and
linkers (ld, lld, mold). Additional workflows: `symbols-check.yml` (ABI validation on public
header changes), `pre-commit.yaml`, `tics.yml`. `.github/filters.yaml` controls which jobs run for
which file changes. All PRs must pass the spread matrix before merging.

Pre-commit hooks (`.pre-commit-config.yaml`) cover trailing whitespace, end-of-file fixers,
Markdown formatting (mdformat), and a large-files check. Run `pre-commit run --all-files`.

## General principles

- **Consistency is paramount**: Follow the existing style of the files you're modifying. When in doubt, local consistency wins.
- **Readability over cleverness**: Write code that is easy to understand and maintain.
- **Minimal changes**: Make the smallest change that fully solves the problem — no incidental reformatting, whitespace normalization, or unrelated comment tweaks. If a formatter touches untouched lines, revert those hunks or commit them separately.
- **Comments are for non-obvious intent**: Do not add comments that merely restate code. Add comments only when they capture rationale, invariants, caveats, or constraints that are not clear from the code itself.
- **Evaluate specialization early**: When a task arrives, consider whether it is a good candidate for a dedicated skill or specialized agent before proceeding with general-purpose implementation.
- **Ask to align**: Ask the user questions to align and ground yourself often. When requirements are ambiguous, scope is unclear, or multiple reasonable approaches exist, confirm with the user rather than guessing.

## Communication and evidence

- In commentary (status updates, summaries, and recommendations), when asserting facts about external codebases, standards/specs, tools, or other non-local sources, include a precise reference so the claim is verifiable.
- Prefer primary sources and stable pointers: repository/file/line, commit SHA, official docs URL, specification section, or release notes.
- If you cannot provide a reliable reference for an external factual claim, qualify it as uncertain and avoid presenting it as established fact.

The full C++ style guide is `doc/sphinx/contributing/reference/cppguide.md`; day-to-day highlights
are in `cpp.instructions.md`.

## Research & prior art

Some tasks — surveying how other compositors solve a problem, evaluating a design before
implementing it, or gathering background on an unfamiliar subsystem — call for dedicated research
rather than jumping straight into code.

- Before starting new research, check whether a prior-art survey or design note already exists (ask the user or search the repo). Update or extend existing research whenever possible — it is almost always cheaper than starting from scratch — and proactively suggest doing so for any open research problem.
- When you produce research, save it to a reusable document, citing primary sources precisely (repository, file, line, commit SHA) so claims can be re-verified.
- For surveys spanning multiple ecosystems or projects (e.g. comparing scene-graph designs across compositors), run a small number of `research` agents in parallel — one per distinct area — each citing primary sources precisely.

## Pull requests & review

PRs use `.github/pull_request_template.md`: link the issue (**Closes #???**), summarize what's new,
give test steps, and complete the checklist. Reviewers focus on:

- **ABI compatibility**: Are symbol maps and Debian symbols updated? Do MirAL headers stay ABI-stable?
- **Protocol correctness**: Do Wayland protocol implementations follow the spec? Are client errors reported on the correct interface?
- **Exception safety**: Does the change maintain at least basic exception safety?
- **Test coverage**: Are new behaviors covered? Are existing tests still valid?
- **Simplicity**: Prefer `value_or()` over conditionals, `std::erase_if` over manual loops, clear names over clever tricks.
- **PR metadata quality**: When reviewing a pull request, evaluate whether the current title and description accurately reflect the actual content and scope; suggest concrete improvements when they do not.

## Commit and session hygiene

- Run `pre-commit run` after `git add` and before `git commit` to surface linting, formatting, and static-check issues in the staged change set.
- If `pre-commit run` reports failures or applies fixes, restage as needed and re-run checks before committing.
- At the end of a session, briefly consider whether user guidance revealed gaps in these LLM instructions and, when it did, propose or apply focused instruction improvements.

## Additional resources

- [Hacking Guide](../HACKING.md)
- [Getting Started with Mir](../doc/sphinx/tutorial/getting-started-with-mir.md)
- [Continuous Integration Reference](../doc/sphinx/contributing/reference/continuous-integration.md)

## Trusting these instructions

Trust the guidance in this file and in `.github/instructions/`. Only fall back to searching or
exploring the codebase when the information here is incomplete, or when you find it to be in error —
in which case prefer fixing the instructions as part of your change.
