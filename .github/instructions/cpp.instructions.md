---
applyTo: '**/*.h,**/*.cpp'
description: C++ coding standards, common patterns, and logging for Mir source and headers.
---

# Mir C++ guidelines

Mir follows a comprehensive C++ style guide in
`doc/sphinx/contributing/reference/cppguide.md`. When in doubt, match the style of the
surrounding code. Highlights:

## File headers

All source files must begin with the copyright header:

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

## Naming conventions

- **Classes/Structs/Types**: `PascalCase` (e.g., `MyExcitingClass`, `UrlTableProperties`)
- **Functions**: `snake_case` (e.g., `add_table_entry()`, `open_file()`)
- **Variables / Constants**: `snake_case` (e.g., `my_variable`, `default_width`)
- **Macros**: `UPPER_CASE` (e.g., `MY_MACRO`) — but avoid macros when possible
- **Namespaces**: `snake_case` (e.g., `my_awesome_project`)

## Namespaces

- `mir::` — root namespace
- `mir::geometry::` — geometric types (Point, Size, Rectangle, etc.)
- `mir::graphics::`, `mir::input::`, `mir::scene::` — component namespaces
- `miral::` — Abstraction layer APIs
- **Unnamed namespaces**: Encouraged in `.cpp` files for internal linkage; **never** in headers
- **Using directives**: Allowed in `.cpp` files, avoid in headers
- **Namespace content**: Not indented

### Common namespace aliases

Use these standard aliases in `.cpp` files for brevity:

- `namespace geom = mir::geometry;`
- `namespace mg = mir::graphics;`
- `namespace mgg = mir::graphics::gbm;`
- `namespace mga = mir::graphics::atomic;`
- `namespace mi = mir::input;`
- `namespace ms = mir::scene;`
- `namespace mf = mir::frontend;`
- `namespace msh = mir::shell;`
- `namespace mc = mir::compositor;`
- `namespace mw = mir::wayland;` (in Wayland-related files)
- `namespace mlc = miral::live_config;` (in live config-related files)

## File organization

- **Header files**: Use `.h` extension with proper header guards
- **Implementation files**: Use `.cpp` extension
- **Include order**: See `cppguide.md`

## Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 120 characters
- **Braces**: Opening brace on new line for functions, classes, namespaces, and branches
- **Function declarations**: Return type or `auto` on same line as function name
- **Pointer/Reference placement**: Asterisk/ampersand adjacent to type (e.g., `char* c`, `string const& str`)

## C++ features

- **Use C++23 features** where appropriate
- **Constructors**: Use `explicit` for single-argument constructors
- **Initialization**: Prefer `{}` initialization (e.g., `int n{5}`)
- **Null pointers**: Use `nullptr` instead of `NULL` or `0`
- **Const correctness**: Use `const` wherever possible; prefer `int const*` over `const int*`
- **Smart pointers**: Prefer smart pointers over raw pointers for ownership
- **Auto keyword**: Use `auto` for complex types and iterators when it improves readability, as well as new-style function declarations with `auto`
- **Optional values**: Use `std::optional<T>` for values that may or may not be present; use `std::nullopt` to represent absence
- **Enum classes**: Prefer `enum class` over plain `enum` for type safety
- **Time types**: Use `mir::time::Timestamp` for timestamps and `mir::time::Duration` for durations (from `mir/time/types.h`)
- **Noexcept**: Mark move operations as `noexcept`. Avoid on destructors — they are usually implicitly `noexcept`, and explicitly marking one `noexcept` when it is not will cause `std::terminate()`

## Classes

- **Access specifiers order**: `public:`, then `protected:`, then `private:`
- **Member declaration order**: Typedefs and Enums, Constants, Constructors, Destructor, Methods, Data members
- **Copy/Move operations**: Explicitly delete or implement as needed
- **Virtual destructors**: Use when class has virtual methods
- **Override keyword**: Always use `override` when overriding virtual methods
- **Deleted functions**: Use `= delete` to explicitly disable copy/move operations when not needed

## Error handling

- **Exceptions**: Functions should throw exceptions when they cannot meet post-conditions
- **Exception safety**: Provide at least the basic exception safety guarantee
- **Preconditions**: May be verified using `fatal_error_abort()` for fatal errors. Do not use `assert()` — its behavior differs between debug and release builds.
- **Pointers**: Assumed to be valid unless explicitly documented otherwise
- **Wayland protocol errors**: Client-caused errors (invalid input, protocol violations) should be reported by throwing `mw::ProtocolError` (see the Wayland protocol instructions)

## Comments

- **File headers**: Include copyright, license, and description
- **Class comments**: Describe purpose and usage; include Doxygen-style documentation in headers. When adding a new class, mention what version of MirAL it was added in (e.g., "\\remark Since MirAL X.Y").
- **Function comments**: Describe what the function does, not how (unless complex)
- **TODO comments**: Use format `// TODO: Description`
- **Code documentation**: Comment tricky or non-obvious code
- **Avoid obvious comments**: Don't state what the code clearly does

## Best practices

- **Keep functions small**: Prefer functions under 40 lines
- **Avoid global state**: Minimize use of global variables
- **Forward declarations**: Prefer forward declarations over `#include` in headers whenever a type is only used by pointer or reference; only include the full definition in the `.cpp`. For stream types, prefer `<iosfwd>` over `<ostream>` or `<iostream>` in headers.
- **Public API changes**: When adding, removing, or changing symbols in public headers, update the corresponding `symbols.map` and Debian symbols file — see the ABI & symbols instructions for the procedure.
- **Inline functions**: Only for small functions (≤10 lines)
- **Preprocessor macros**: Avoid when possible; prefer inline functions, enums, and const variables
- **Streams**: Use only for logging, prefer `printf`-style for other I/O
- **Reference parameters**: Input parameters should be `const` references or values; output parameters are rare and should use non-const references when modifying inputs, or return `std::pair`/`std::tuple` for multiple outputs
- **Modern idioms**:
  - **Optionals**: Prefer `value_or()` over `if (opt) ... else ...` for defaults; chain with `.transform(fn).value_or(fallback)` for map-then-default
  - **Container erasure**: Use `std::erase_if(container, predicate)` over manual iterator loops
  - **Ranges**: Use `std::ranges::find_if`, `std::ranges::any_of`, `std::ranges::reverse_view` over raw iterator algorithms; use range adaptors like `| std::views::filter(...)`, `| std::views::transform(...)`, `| std::views::keys` for pipeline-style data processing
  - **`std::expected`**: For operations that return a value or an error code (see `ext_image_capture_v1.cpp`)
  - **`std::variant` + `std::visit`**: For type-safe unions, especially in platform construction (see `gbm-kms/platform.cpp`)
- **Legacy workarounds**: When a workaround is needed for an older supported distro, mark it with a `LEGACY(XX.YY)` comment (e.g., `// LEGACY(24.04): remove once ranges::to is available`)

## Common patterns

### Thread safety

- Use `mir::Synchronised<T>` for thread-safe access to shared data — it combines a mutex with the data it protects
- Prefer `std::lock_guard` for simple lock scoping; use `std::unique_lock` when you need more control (e.g., condition variables)
- Observer pattern is widely used — classes ending in `Observer` or `Listener` implement notification interfaces. Never assume listeners are running on the same thread.
- Use `Executor::spawn()` to dispatch work to the Wayland thread from observer threads

### Resource management

- Use RAII for all resource management
- Prefer `std::shared_ptr` and `std::unique_ptr` for ownership
- Use `std::weak_ptr` in non-Wayland observer/callback scenarios to avoid cycles; for Wayland objects, use `wayland::Weak<T>` instead (see the Wayland protocol instructions)

### Interface abstractions

- Pure virtual interfaces (ending with `= 0`) define contracts between components
- Implementation classes typically live in `src/` while interfaces are in `include/`

### Geometry type system

Mir uses a tag-based strong-type system for geometry in `include/core/mir/geometry/`:

- **Strong-typed scalars**: `Value<T, Tag>` wraps a numeric value with a tag type (`XTag`, `YTag`, `WidthTag`, `HeightTag`, `DeltaXTag`, `DeltaYTag`, `StrideTag`)
- **Aliases**: `X`, `Y`, `Width`, `Height`, `DeltaX`, `DeltaY` are `Value<int, *Tag>`; float variants are `XF`, `WidthF`, etc.
- **Composite types**: `Point{X, Y}`, `Size{Width, Height}`, `Displacement{DeltaX, DeltaY}`, `Rectangle{Point, Size}`
- **Generic vs concrete**: `generic::Point<T>` is the template; `Point` is `generic::Point<int>`, `PointF` is `generic::Point<float>`

**Type-safe arithmetic** — the compiler prevents mixing incompatible axes:

- ✅ `X + DeltaX → X`, `Point + Displacement → Point`, `Point - Point → Displacement`
- ❌ `X + DeltaY` — **compile error**
- ❌ `Width + Height` — **compile error**

**Conversion functions** for cross-tag conversions (explicit only): `as_width(DeltaX)`, `as_delta(Width)`, `as_point(Size)`, `as_displacement(Point)`, etc. Use `namespace geom = mir::geometry;` for brevity.

### Server component / dependency injection

Mir's server uses a **CachedPtr + virtual factory** pattern for DI:

- **`CachedPtr<T>`** (`include/common/mir/cached_ptr.h`): Lazy single-instance factory backed by a `weak_ptr`. First call creates and caches; subsequent calls return cached instance.
- **`the_*()` methods**: Each component in `DefaultServerConfiguration` has a factory method (e.g., `the_shell()`, `the_compositor()`, `the_display()`).
- **`override_the_*(Builder<T>)`**: Replaces the default factory for a component entirely.
- **`wrap_*(Wrapper<T>)`**: Decorates an existing component (receives the original, returns a wrapper). Used for the decorator pattern.

**MirRunner functor composition**: All miral configurators implement `void operator()(mir::Server&)` and are passed to `MirRunner::run_with({...})`. This is how shells compose window management policies, renderers, authorizers, etc.

### Platform plugin system

Graphics backends are loaded as shared library plugins:

- **`DisplayPlatform`** manages output hardware (modesetting, CRTC). **`RenderingPlatform`** manages buffer allocation and GL rendering. They are separate interfaces.
- Each platform module exports C-linkage entry points: `probe_display_platform()`, `create_display_platform()`, `probe_rendering_platform()`, `create_rendering_platform()`.
- **Probe pattern**: Returns `SupportedDevice` with a priority (`unsupported` < `dummy` < `supported` < `nested` < `best`). The server selects the highest-scoring platform per hardware device.
- Platform modules use `add_library(... MODULE)` in CMake with custom suffix `.so.${ABI_VERSION}`.

## Logging

API (in `include/core/mir/log.h`): `log_info()`, `log_warning()`, `log_error()`, `log_debug()`,
`log_critical()`.

- These are **inline functions** (not macros) in an anonymous namespace, gated by `#ifdef MIR_LOG_COMPONENT`. They automatically inject the component name — callers don't pass it.
- The component name comes from `MIR_LOG_COMPONENT_FALLBACK` set per-library in CMake (e.g., `add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mirserver")`). This is the **preferred** approach — a consistent library-wide default without per-file noise.
- `#define MIR_LOG_COMPONENT "name"` at the top of a `.cpp` file is an **override** for use only when a file belongs to a distinct named subsystem (e.g., a specific platform backend compiled into a larger server library). Do not define it in every `.cpp` file.
- Logging uses **printf-style** format specifiers (`%s`, `%d`, `%x`, etc.) via `va_list` — **not** `std::format` `{}` placeholders. There are also `std::string` overloads (e.g., `log_error(std::string const&)`) for pre-formatted messages.

```cpp
#include <mir/log.h>

log_info("Started %s on fd %d", name.c_str(), fd);      // printf-style
log_warning("Unexpected state: %s", state_str.c_str());  // printf-style
log_error(std::format("Device {}: failed", handle));      // pre-formatted string
```

- `log_error`: operation **failed and was abandoned** — the server continues but the specific action is lost
- `log_warning`: **unexpected but handled** — e.g., invalid config value, missing optional resource
- When passing a pre-formatted `std::string` (e.g., via `std::format`), use the `std::string const&` overload directly — don't mix printf specifiers with `std::format` output
