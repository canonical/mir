# Mir Copilot Instructions

This document provides essential knowledge for working with the Mir codebase, including architecture, coding standards, and workflows.

## Overview

Mir is a Wayland compositor library written in C++23 with some Rust components. It provides a stable ABI layer (mirAL) for building shells, handling graphics/input hardware abstraction, and window management.

## Architecture

### Core Layers

- **include/**: Public API headers with minimal implementation details. Interfaces here should not expose platform-specific types.
- **src/**: Implementation organized by component. Internal headers stay within their component directory.
- **miral/** (Mir Abstraction Layer): ABI-stable layer in `include/miral/miral/` providing shell-building APIs like `MirRunner`, `WindowManagementPolicy`, window managers, and accessibility features.
- **Platform backends**: `src/platforms/{gbm-kms,atomic-kms,x11,wayland,eglstream-kms,virtual}` - hardware abstraction layers for different graphics/input systems.
- **Input platforms**: `evdev` (C++), `evdev-rs` (Rust with CXX bindings)

### Key Components

- **Server** (`src/server/`): Core compositor services (scene, compositor, graphics, frontend_xwayland)
- **Renderers** (`src/renderers/gl/`): GL-based rendering implementations
- **Tests** (`tests/`): Unit tests, integration tests, performance tests using gtest/gmock
- **Documentation** (`doc/sphinx/`): Sphinx-based docs using the Canonical theme, hosted on Read the Docs and proxied via `canonical.com/mir/docs/`

## Build System & CI

### Building Mir

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

### Running Tests

```sh
cd build
# Run all tests
ctest --output-on-failure
# Run a specific test binary with gtest filter
bin/mir_unit_tests --gtest_filter='TestSuite.test_name'
bin/miral-test --gtest_filter='*WindowManager*'
```

### CI Overview

CI is driven by [spread](https://github.com/snapcore/spread) (defined in `spread.yaml`) and runs via `.github/workflows/spread.yml`. The test matrix includes:

- **Distros**: Ubuntu (noble, questing, devel), Debian sid, Fedora (42, 43, rawhide), Alpine (3.21–edge)
- **Architectures**: amd64, arm64, armhf
- **Sanitizer builds**: AddressSanitizer, ThreadSanitizer, UBSanitizer (each with both GCC and Clang)
- **Linkers**: ld, lld, mold
- **Additional workflows**: `symbols-check.yml` (ABI validation on public header changes), `pre-commit.yaml`, `tics.yml` (code quality)

The `.github/filters.yaml` file controls which CI jobs trigger for which file changes.

All PRs must pass the spread CI matrix before merging.

## Coding Standards

Mir follows a comprehensive C++ style guide documented in [`doc/sphinx/contributing/reference/cppguide.md`](../doc/sphinx/contributing/reference/cppguide.md). Below are key highlights:

### File Headers

All source files must include a copyright header:

```cpp
/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
```

### General Principles

- **Consistency is paramount**: Follow existing code style in files you're modifying
- **Readability over cleverness**: Write code that is easy to understand and maintain
- **Minimal changes**: Make the smallest possible changes to achieve your goal

### Naming Conventions

- **Classes/Structs/Types**: `PascalCase` (e.g., `MyExcitingClass`, `UrlTableProperties`)
- **Functions**: `snake_case` (e.g., `add_table_entry()`, `open_file()`)
- **Variables**: `snake_case` (e.g., `my_variable`, `table_name`)
- **Constants**: `snake_case` (e.g., `default_width`, `max_connections`)
- **Macros**: `UPPER_CASE` (e.g., `MY_MACRO`) - but avoid macros when possible
- **Namespaces**: `snake_case` (e.g., `my_awesome_project`)

### Namespaces

- `mir::` - root namespace
- `mir::geometry::` - geometric types (Point, Size, Rectangle, etc.)
- `mir::graphics::`, `mir::input::`, `mir::scene::` - component namespaces
- `miral::` - Abstraction layer APIs
- **Unnamed namespaces**: Encouraged in `.cpp` files for internal linkage
- **No unnamed namespaces in headers**
- **Using directives**: Allowed in `.cpp` files, avoid in headers
- **Namespace content**: Not indented

### Common Namespace Aliases

Use these standard aliases in `.cpp` files for brevity:

- `namespace geom = mir::geometry;`
- `namespace mg = mir::graphics;`
- `namespace mi = mir::input;`
- `namespace ms = mir::scene;`
- `namespace mf = mir::frontend;`
- `namespace msh = mir::shell;`
- `namespace mc = mir::compositor;`
- `namespace mw = mir::wayland;` (in Wayland-related files)

### File Organization

- **Header files**: Use `.h` extension with proper header guards
- **Implementation files**: Use `.cpp` extension
- **Include order**:
  1. Corresponding header file (for .cpp files)
  1. Project's public headers
  1. Project's private headers
  1. Other libraries' headers
  1. C system headers
  1. C++ standard library headers

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 120 characters
- **Braces**: Opening brace on new line for functions, classes, namespaces, and branches
- **Function declarations**: Return type or `auto` on same line as function name
- **Pointer/Reference placement**: Asterisk/ampersand adjacent to type (e.g., `char* c`, `string const& str`)

### C++ Features

- **Use C++23 features** where appropriate
- **Constructors**: Use `explicit` for single-argument constructors
- **Initialization**: Prefer `{}` initialization (e.g., `int n{5}`)
- **Null pointers**: Use `nullptr` instead of `NULL` or `0`
- **Const correctness**: Use `const` wherever possible; prefer `int const*` over `const int*`
- **Smart pointers**: Prefer smart pointers over raw pointers for ownership
- **Auto keyword**: Use `auto` for complex types and iterators when it improves readability, as well as new-style function declarations with `auto`
- **Optional values**: Use `std::optional<T>` for values that may or may not be present; use `std::nullopt` to represent absence
- **Enum classes**: Prefer `enum class` over plain `enum` for type safety
- **Time types**: Use `std::chrono::nanoseconds` for timestamps and `std::chrono::milliseconds` for durations
- **Noexcept**: Mark destructors and move operations as `noexcept` where appropriate

### Classes

- **Access specifiers order**: `public:`, then `protected:`, then `private:`
- **Member declaration order**:
  1. Typedefs and Enums
  1. Constants
  1. Constructors
  1. Destructor
  1. Methods
  1. Data members
- **Copy/Move operations**: Explicitly delete or implement as needed
- **Virtual destructors**: Use when class has virtual methods
- **Override keyword**: Always use `override` when overriding virtual methods
- **Deleted functions**: Use `= delete` to explicitly disable copy/move operations when not needed:
  ```cpp
  MyClass(MyClass const&) = delete;
  MyClass& operator=(MyClass const&) = delete;
  ```

### Error Handling

- **Exceptions**: Functions should throw exceptions when they cannot meet post-conditions
- **Exception safety**: Provide at least the basic exception safety guarantee
- **Preconditions**: May be verified using `fatal_error_abort()` for fatal errors. Do not use `assert()` as its behavior differs between debug and release builds.
- **Pointers**: Assumed to be valid unless explicitly documented otherwise
- **Wayland protocol errors**: Client-caused errors (invalid input, protocol violations) should be reported by throwing `mw::ProtocolError` — see [Protocol Correctness](#protocol-correctness) for details.
- **Logging**: `log_error()` for operations that failed and were abandoned; `log_warning()` for unexpected but handled situations — see [Logging](#logging) for the full API.

### Comments

- **File headers**: Include copyright, license, and description
- **Class comments**: Describe purpose and usage; include Doxygen-style documentation in headers
- **Function comments**: Describe what the function does, not how (unless complex)
- **TODO comments**: Use format `// TODO: Description`
- **Code documentation**: Comment tricky or non-obvious code
- **Avoid obvious comments**: Don't state what the code clearly does

### Best Practices

- **Keep functions small**: Prefer functions under 40 lines
- **Avoid global state**: Minimize use of global variables
- **Forward declarations**: Use when possible to reduce header dependencies
- **Inline functions**: Only for small functions (≤10 lines)
- **Preprocessor macros**: Avoid when possible; prefer inline functions, enums, and const variables
- **Streams**: Use only for logging, prefer `printf`-style for other I/O
- **Reference parameters**: Input parameters should be `const` references or values; output parameters are rare and should use non-const references when modifying inputs, or return std::pair/std::tuple for multiple outputs
- **Modern idioms**:
  - **Optionals**: Prefer `value_or()` over `if (opt) ... else ...` for defaults; chain with `.transform(fn).value_or(fallback)` for map-then-default
  - **Container erasure**: Use `std::erase_if(container, predicate)` over manual iterator loops
  - **Ranges**: Use `std::ranges::find_if`, `std::ranges::any_of`, `std::ranges::reverse_view` over raw iterator algorithms; use range adaptors like `| std::views::filter(...)`, `| std::views::transform(...)`, `| std::views::keys` for pipeline-style data processing
  - **`std::expected`**: Used for operations that return a value or an error code (see `ext_image_capture_v1.cpp`)
  - **`std::variant` + `std::visit`**: Used for type-safe unions, especially in platform construction (see `gbm-kms/platform.cpp`)
- **Legacy workarounds**: When a workaround is needed for an older supported distro, mark it with a `LEGACY(XX.YY)` comment (e.g., `// LEGACY(24.04): remove once ranges::to is available`)
- **`extern "C"` blocks**: Do not include C++ standard library headers (e.g., `<cstdlib>`) inside `extern "C"` blocks — this causes compilation errors or ODR issues. Use `<stdlib.h>` if a C header is needed inside C linkage.

## Common Patterns

### Thread Safety

- Use `mir::Synchronised<T>` for thread-safe access to shared data - it combines a mutex with the data it protects
- Prefer `std::lock_guard` for simple lock scoping
- Use `std::unique_lock` when you need more control (e.g., condition variables)
- Observer pattern is widely used - classes ending in `Observer` or `Listener` implement notification interfaces
- Use `wayland::Weak<T>` for cross-thread references to Wayland objects (see [Wayland Object Lifecycle](#wayland-object-lifecycle))
- Use `Executor::spawn()` to dispatch work to the Wayland thread from observer threads

### Resource Management

- Use RAII for all resource management
- Prefer `std::shared_ptr` and `std::unique_ptr` for ownership
- Use `std::weak_ptr` in non-Wayland observer/callback scenarios to avoid cycles
- For Wayland objects, use `wayland::Weak<T>` instead (see [Wayland Object Lifecycle](#wayland-object-lifecycle))

### Interface Abstractions

- Pure virtual interfaces (ending with `= 0`) define contracts between components
- Implementation classes typically live in `src/` while interfaces are in `include/`

### Geometry Type System

Mir uses a tag-based strong-type system for geometry in `include/core/mir/geometry/`:

- **Strong-typed scalars**: `Value<T, Tag>` wraps a numeric value with a tag type (`XTag`, `YTag`, `WidthTag`, `HeightTag`, `DeltaXTag`, `DeltaYTag`, `StrideTag`)
- **Aliases**: `X`, `Y`, `Width`, `Height`, `DeltaX`, `DeltaY` are `Value<int, *Tag>`; float variants are `XF`, `WidthF`, etc.
- **Composite types**: `Point{X, Y}`, `Size{Width, Height}`, `Displacement{DeltaX, DeltaY}`, `Rectangle{Point, Size}`
- **Generic vs concrete**: `generic::Point<T>` is the template; `Point` is `generic::Point<int>`, `PointF` is `generic::Point<float>`

**Type-safe arithmetic** — the compiler prevents mixing incompatible axes:

- ✅ `X + DeltaX → X`, `Point + Displacement → Point`, `Point - Point → Displacement`
- ❌ `X + DeltaY` — **compile error**
- ❌ `Width + Height` — **compile error**

**Conversion functions** for cross-tag conversions (explicit only): `as_width(DeltaX)`, `as_delta(Width)`, `as_point(Size)`, `as_displacement(Point)`, etc.

Use `namespace geom = mir::geometry;` for brevity.

### Server Component / Dependency Injection

Mir's server uses a **CachedPtr + virtual factory** pattern for DI:

- **`CachedPtr<T>`** (`include/common/mir/cached_ptr.h`): Lazy single-instance factory backed by a `weak_ptr`. First call creates and caches; subsequent calls return cached instance.
- **`the_*()` methods**: Each component in `DefaultServerConfiguration` has a factory method (e.g., `the_shell()`, `the_compositor()`, `the_display()`).
- **`override_the_*(Builder<T>)`**: Replaces the default factory for a component entirely.
- **`wrap_*(Wrapper<T>)`**: Decorates an existing component (receives the original, returns a wrapper). Used for the decorator pattern.

**MirRunner functor composition**: All miral configurators implement `void operator()(mir::Server&)` and are passed to `MirRunner::run_with({...})`. This is how shells compose window management policies, renderers, authorizers, etc.

### Platform Plugin System

Graphics backends are loaded as shared library plugins:

- **`DisplayPlatform`** manages output hardware (modesetting, CRTC). **`RenderingPlatform`** manages buffer allocation and GL rendering. They are separate interfaces.
- Each platform module exports C-linkage entry points: `probe_display_platform()`, `create_display_platform()`, `probe_rendering_platform()`, `create_rendering_platform()`.
- **Probe pattern**: Returns `SupportedDevice` with a priority (`unsupported` < `dummy` < `supported` < `nested` < `best`). The server selects the highest-scoring platform per hardware device.
- Platform modules use `add_library(... MODULE)` in CMake with custom suffix `.so.${ABI_VERSION}`.

## ABI Stability & Symbol Management

MirAL provides a stable ABI. Any changes to the public API must be reflected in symbol maps and Debian symbol files.

- Each library has a `symbols.map` (e.g., `src/miral/symbols.map`) with versioned stanzas (e.g., `MIRAL_5.0 { ... }`).
- Corresponding Debian symbol files (e.g., `debian/libmiral7.symbols`) must be updated in lockstep.
- MirAL headers (`include/miral/miral/`) must NOT expose implementation details: no templates with logic in headers, no inline functions that depend on internal types. Use the pImpl pattern.
- Do not put "deducing this" or other constructs that require definitions in headers — MirAL headers must remain ABI-stable.

### When and How to Bump ABIs

#### miral / miroil — adding a new symbol (non-breaking)

1. Add the new method/class to the public header and implementation
1. If not already bumped this release cycle, bump `MIRAL_VERSION_MINOR` in `src/CMakeLists.txt` (or `MIROIL_VERSION_MINOR` in `src/miroil/CMakeLists.txt`)
1. `cmake --build <BUILD_DIR> --target generate-miral-symbols-map` (or `generate-miroil-symbols-map`)
1. Verify with `git diff src/miral/symbols.map`
1. `cmake --build <BUILD_DIR> --target regenerate-miral-debian-symbols` (or `regenerate-miroil-debian-symbols`)

No `*_ABI` bump is required for additive changes.

#### miral / miroil — removing or changing a symbol (ABI break)

1. Make the destructive change
1. Bump `MIRAL_VERSION_MAJOR` in `src/CMakeLists.txt`, reset `MIRAL_VERSION_MINOR` and `MIRAL_VERSION_PATCH` to 0
1. Bump `MIRAL_ABI` in `src/miral/CMakeLists.txt`
1. Run `generate-miral-symbols-map` then `regenerate-miral-debian-symbols`
1. Run `./tools/update_package_abis.sh` to update Debian package names (control file, `.install` file renames)

#### mirserver — any symbol change (additive or breaking)

mirserver has no stable ABI promise. **Every** symbol change requires bumping `MIRSERVER_ABI`:

1. Make the change
1. Bump `MIRSERVER_ABI` in `src/server/CMakeLists.txt`
1. Bump the minor number in `project(Mir VERSION X.Y.Z)` in the **top-level** `CMakeLists.txt` (the stanza name `MIR_SERVER_INTERNAL_X.Y` derives from this)
1. `cmake --build <BUILD_DIR> --target generate-mirserver-symbols-map`
1. Run `./tools/update_package_abis.sh` (handles `debian/control` and `.install` file renames)

**Note**: there is no `regenerate-mirserver-debian-symbols` target — mirserver debian packaging is managed by `update_package_abis.sh`.

#### General rules

- All version bumps are once-per-release-cycle — check if someone already bumped before you do.
- CI (`symbols-check.yml`) catches added/removed symbols but does **not** detect changed signatures — those must be moved to a new stanza manually.
- `tools/update_package_abis.sh` handles `debian/control` and `.install` file renames for all libraries. It does not modify `symbols.map`, CMake variables, or RPM packaging.

See [How to Update Symbol Maps](../doc/sphinx/contributing/how-to/update-symbols-map.md) and [DSO Versioning Guide](../doc/sphinx/contributing/reference/dso-versioning-guide.md) for full details.

## Testing

### Framework

- **gtest/gmock** for unit and integration tests
- Test file naming: `test_*.cpp` (e.g., `test_floating_window_manager.cpp`)
- Mock classes: `Mock*` prefix (e.g., `MockInputDispatcher` in `mir::test::doubles`)
- Stub classes: `Stub*` or `Fake*` prefix for test doubles

### Test Organization

- `tests/unit-tests/` - Component unit tests
- `tests/integration-tests/` - Cross-component integration tests
- `tests/mir_test_doubles/` - Reusable mocks and test utilities
- `mir_test_framework::WindowManagementTestHarness` - Base for window manager tests

### Example Test Pattern

```cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class MyComponentTest : public testing::Test { /* ... */ };

TEST_F(MyComponentTest, descriptive_test_name)
{
    // Arrange, Act, Assert
}
```

### GMock Matchers

Use GMock matchers with `EXPECT_THAT` and `ASSERT_THAT` for expressive assertions:

```cpp
EXPECT_THAT(value, Eq(expected));
EXPECT_THAT(result, Ne(unwanted));
EXPECT_THAT(count, Gt(0));
EXPECT_THAT(collection, Contains(item));
```

### Flaky Test Guidelines

Intermittent test failures are tracked in [issue #4507](https://github.com/canonical/mir/issues/4507). When writing or fixing tests:

- Use `ASSERT_*` only for preconditions and assumptions — if the assertion fails, the rest of the test is meaningless (e.g., a wait that must succeed before subsequent checks). Use `EXPECT_*` for everything else.
- Use reasonable timeouts: 1–2 seconds for callback waits is typical; 5+ seconds is excessive.
- Do not rely on tight wall-clock margins (e.g., 10ms scheduling windows) for timing-sensitive tests. Prefer controllable/fake clocks, or wait with a generous timeout and assert on measured elapsed time.
- When fixing a flaky test, address the root cause. Simply increasing a timeout is not a fix.

### WLCS (Wayland Conformance) Tests

- WLCS acceptance tests live in `tests/acceptance-tests/wayland/`.
- **`expected_wlcs_failures.list`** tracks known failures (XFAIL). When removing a protocol or fixing a bug, update this list accordingly.
- Skipped tests (e.g., protocol unavailable) that are listed as XFAIL will show as "unexpected pass" — remove them from the list.
- WLCS is a system package; version-dependent features are gated with `WLCS_VERSION` checks in CMake and `WLCS_DISPLAY_SERVER_VERSION` compile-time guards.

## Wayland Protocol Integration

When integrating a new Wayland protocol:

1. **Place protocol XML** in `wayland-protocols/` directory

1. **Generate wrapper code** using the `mir_generate_protocol_wrapper()` CMake function (defined in `cmake/MirCommon.cmake`):

```cmake
# In the appropriate CMakeLists.txt (usually src/wayland/CMakeLists.txt)
mir_generate_protocol_wrapper(mirwayland "zwp_" path/to/protocol.xml)
```

This generates a `*_wrapper.h` (class declarations) and `*_wrapper.cpp` (thunks, message handling) from the XML.

3. **Implement the protocol** in `src/server/frontend_wayland/`:

   - Create implementation files in the `mir::frontend` namespace
   - Inherit from the generated `wayland::ProtocolName::Global` (for bindable globals) or `wayland::ProtocolName` (for resources)
   - Override the pure virtual request methods generated from the protocol XML
   - Follow existing patterns (see `foreign_toplevel_manager_v1.cpp`, `ext_image_capture_v1.cpp`, etc.)

1. **Create a factory function** to instantiate the protocol global:

   - Expose a `create_*_manager()` or `create_*_global()` function
   - Pass required dependencies via `WaylandConnector::Context` (e.g., `MainLoop`, `Executor`, `Shell`)
   - Return `std::shared_ptr<wayland::ProtocolName::Global>`

1. **Integrate with WaylandConnector**:

   - If the protocol needs existing server components (MainLoop, Shell, etc.), access them through `WaylandConnector::Context`
   - If the protocol requires a new server component, add it to `DefaultServerConfiguration` and expose via `Server`
   - Register the protocol global with the wayland display in the appropriate initialization code

1. **Hook into MirAL** (if needed):

   - Integrate with `miral::WaylandExtensions` in your shell to enable/disable the protocol

### Wayland Object Lifecycle

The generated code manages object lifecycle automatically:

- **`wayland::Resource`** is the base class for all Wayland protocol objects. It wraps a `wl_resource*` and sets up a destruction thunk via `wl_resource_set_implementation()`.
- **`wayland::Global`** is the base class for bindable globals (advertised to all clients). It wraps `wl_global*` and handles version negotiation in the bind callback.
- **`wayland::LifetimeTracker`** provides `destroyed_flag()` and `add_destroy_listener()` for safe cross-thread destruction tracking.
- **`wayland::Weak<T>`** is Mir's alternative to `std::weak_ptr` for Wayland objects. Use it for any cross-thread reference to a Wayland resource — it checks a `destroyed_flag` rather than requiring shared ownership.
- **Version-gated events**: Generated code provides `version_supports_*()` and `send_*_event_if_supported()` methods. Always use the `_if_supported` variant when the event was added in a later protocol version.
- **Destruction is automatic**: When the `wl_resource` is destroyed (by client or server), the generated `resource_destroyed_thunk` deletes the C++ wrapper object.

### Protocol Correctness

- **Input validation**: Validate all client-supplied values at the protocol boundary. Reject invalid inputs (e.g., non-positive dimensions, invalid enums) using `mw::ProtocolError` with the appropriate error code.
- **Error code matching**: The error code must belong to the *interface of the resource* receiving the error. Sending an error code from a different interface will be misinterpreted by clients (e.g., [PR #4890](https://github.com/canonical/mir/pull/4890) — layer-shell error sent on a layer-surface resource).
- **Error reporting**: Throw `ProtocolError` via `BOOST_THROW_EXCEPTION(mw::ProtocolError(...))` or `throw wayland::ProtocolError{...}`. The generated request thunks catch this and call `wl_resource_post_error()` automatically — do not call `wl_resource_post_error()` directly.
- **Lifecycle management**: Wayland objects can be destroyed by the client at any time — use `wayland::Weak<T>` (see [Wayland Object Lifecycle](#wayland-object-lifecycle)).
- **Fix all protocol instances**: When fixing a bug in a protocol handler, check all other protocol implementations for the same bug. Copy-paste is common across xdg-shell stable/v6 and mir_shell implementations (e.g., [PR #4869](https://github.com/canonical/mir/pull/4869)).
- **Configure sequences**: The frontend may receive state updates piecemeal (e.g., "fullscreen", "new size", "activated" separately). Be aware of how configure events batch these changes — see [issue #4700](https://github.com/canonical/mir/issues/4700).
- **Protocol spec compliance**: Always reference the actual Wayland protocol specification for behavioral requirements (e.g., DnD action selection semantics, positioner constraint logic).

## Rust Components

The `evdev-rs` input platform (`src/platforms/evdev-rs/`) is written in Rust and bridges to C++ via [CXX](https://cxx.rs/).

### Safety Standards

- All `unsafe` blocks must have a `// # Safety` comment explaining **why** the invariants are upheld, not just restating what is unsafe. Bad: "This is unsafe because it receives a raw C++ pointer." Good: "The C++ caller guarantees `platform` outlives `self` and is only accessed from the event loop thread."
- Enable the `undocumented_unsafe_blocks` clippy lint — see [issue #4841](https://github.com/canonical/mir/issues/4841).
- The CXX bridge in `src/platforms/evdev-rs/src/ffi.rs` defines the Rust/C++ FFI boundary. Follow existing patterns when adding new FFI functions.

## Logging

### Current API

- `log_info()`, `log_warning()`, `log_error()`, `log_debug()`, `log_critical()` — defined in `include/core/mir/log.h`
- These are **inline functions** (not macros) in an anonymous namespace, gated by `#ifdef MIR_LOG_COMPONENT`. They automatically inject the component name — callers don't pass it.
- The component name comes from `#define MIR_LOG_COMPONENT "name"` at the top of a `.cpp` file, or from `MIR_LOG_COMPONENT_FALLBACK` set per-library in CMake (e.g., `add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mirserver")`).
- Logging uses **printf-style** format specifiers (`%s`, `%d`, `%x`, etc.) via `va_list` — **not** `std::format` `{}` placeholders.
- There are also `std::string` overloads (e.g., `log_error(std::string const&)`) for pre-formatted messages.

### Usage

```cpp
#define MIR_LOG_COMPONENT "my-component"  // optional per-file override
#include <mir/log.h>

log_info("Started %s on fd %d", name.c_str(), fd);      // printf-style
log_warning("Unexpected state: %s", state_str.c_str());  // printf-style
log_error(std::format("Device {}: failed", handle));      // pre-formatted string
```

- `log_error`: operation **failed and was abandoned** — the server continues but the specific action is lost
- `log_warning`: **unexpected but handled** — e.g., invalid config value, missing optional resource
- When passing a pre-formatted `std::string` (e.g., via `std::format`), use the `std::string const&` overload directly — don't mix printf specifiers with `std::format` output

## Configuration System

MirAL includes a live configuration system under active development:

- **`miral::LiveConfig`** / **`miral::LiveConfigIniFile`**: Runtime-reloadable configuration from INI files.

## Documentation

The docs live in `doc/sphinx/` and use the Canonical Sphinx starter pack with MyST Markdown.

### Structure

The docs follow the [Diátaxis](https://diataxis.fr/) framework:

- `tutorial/` — Learning-oriented first steps
- `how-to/` — Task-oriented guides for compositor authors
- `explanation/` — Understanding-oriented discussions
- `reference/` — API reference and technical specs
- `configuring/` — End-user configuration guides
- `contributing/` — Developer contribution guides

### Building Docs

```sh
cd doc/sphinx
make html       # Build HTML docs
make linkcheck  # Validate links
```

For API docs (requires a CMake build with Doxygen):

```sh
MIR_BUILD_API_DOCS=1 MIR_BUILD_DIR=/path/to/build make html
```

### SEO & Metadata

- **Page titles** should include project context for search disambiguation (e.g., "Mir architecture" not just "Architecture").
- **Open Graph**: Configured via `sphinxext-opengraph`. The `ogp_site_url` and `html_baseurl` must include the version path to produce correct canonical/OG URLs.
- **Sitemap**: Generated by `sphinx-sitemap`. Static files (`robots.txt`, `sitemapindex.xml`) must point to the canonical base URL.
- **Redirects**: Managed in `doc/sphinx/redirects.txt` using `sphinx-rerediraffe`. When renaming or moving a page, always add a redirect entry.

### New Documentation Pages

Every new doc page must include:

1. **A MyST anchor** before the title for cross-referencing:
   ```markdown
   (slug-name)=

   # Page Title
   ```
1. **A meta description** in frontmatter (for search engines and social previews):
   ```markdown
   ---
   myst:
     html_meta:
       description: Short description of this page.
   ---
   ```

## Linting

Pre-commit hooks (`.pre-commit-config.yaml`) handle:

- Trailing whitespace
- End-of-file fixers
- Markdown formatting (mdformat)
- Large files check

Run: `pre-commit run --all-files`

## Pull Requests & Review

### PR Template

All PRs use `.github/pull_request_template.md` which requires:

- **Closes #???** — link to the issue being addressed
- **What's new?** — summary of changes
- **How to test** — steps for reviewers to validate
- **Checklist** — tests added, documentation updated

### CI Requirements

- All spread CI tasks must pass (multiple distros, sanitizers, architectures).
- The `symbols-check` workflow must pass for any changes to public headers.
- Pre-commit checks must pass.

### CMake Conventions

- **No GLOB for sources**: All source files are listed explicitly in `add_library()` / `target_sources()`. Never use `file(GLOB ...)` for source files.
- **Test registration**: Use `mir_add_test()` and `mir_discover_tests()` from `cmake/MirCommon.cmake` — not raw `add_test()`.
- **Test binaries**: Use `mir_add_wrapped_executable(target_name NOINSTALL ...)` for test executables.
- **Wayland protocol wrappers**: Use `mir_generate_protocol_wrapper(target prefix xml_file)` — not raw `add_custom_command`.
- **Rust/CXX integration**: Use `add_rust_cxx_library()` from `cmake/RustLibrary.cmake`.
- **ABI version variables**: Each library defines `set(LIB_ABI N)` locally (e.g., `MIRAL_ABI`, `MIRCORE_ABI`, `MIRWAYLAND_ABI`, `MIRSERVER_ABI`). Bump these when breaking ABI.

### Review Focus Areas

Reviewers typically focus on:

- **ABI compatibility**: Are symbol maps and Debian symbols updated? Do MirAL headers stay ABI-stable?
- **Protocol correctness**: Do Wayland protocol implementations follow the spec? Are client errors reported correctly?
- **Exception safety**: Does the change maintain at least basic exception safety?
- **Test coverage**: Are new behaviors covered by tests? Are existing tests still valid?
- **Simplicity**: Prefer `value_or()` over conditionals, `std::erase_if` over manual loops, clear names over clever tricks.

## Additional Resources

- [Full C++ Style Guide](../doc/sphinx/contributing/reference/cppguide.md)
- [Hacking Guide](../HACKING.md)
- [Getting Started with Mir](../doc/sphinx/tutorial/getting-started-with-mir.md)
- [DSO Versioning Guide](../doc/sphinx/contributing/reference/dso-versioning-guide.md)
- [How to Update Symbol Maps](../doc/sphinx/contributing/how-to/update-symbols-map.md)
- [Continuous Integration Reference](../doc/sphinx/contributing/reference/continuous-integration.md)

## Consistency Reminder

When in doubt, follow the style of the code you're working in. Local consistency is important for maintainability.
