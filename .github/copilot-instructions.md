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
- **evdev-rs**: Rust-based input platform using CXX bindings (`src/platforms/evdev-rs/`, `Cargo.toml`).

### Key Components

- **Server** (`src/server/`): Core compositor services (scene, compositor, graphics, frontend_xwayland)
- **Renderers** (`src/renderers/gl/`): GL-based rendering implementations
- **Tests** (`tests/`): Unit tests, integration tests, performance tests using gtest/gmock

## Key CMake Patterns

- `mir_add_wrapped_executable()`: Creates executables with wrapper scripts

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
  2. Project's public headers
  3. Project's private headers
  4. Other libraries' headers
  5. C system headers
  6. C++ standard library headers

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
  2. Constants
  3. Constructors
  4. Destructor
  5. Methods
  6. Data members
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
- **Preconditions**: May be verified using `assert` but are not required to be checked
- **Pointers**: Assumed to be valid unless explicitly documented otherwise

### Comments

- **File headers**: Include copyright, license, author, and description
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

## Common Patterns

### Thread Safety

- Use `mir::Synchronised<T>` for thread-safe access to shared data - it combines a mutex with the data it protects
- Prefer `std::lock_guard` for simple lock scoping
- Use `std::unique_lock` when you need more control (e.g., condition variables)
- Observer pattern is widely used - classes ending in `Observer` or `Listener` implement notification interfaces

### Resource Management

- Use RAII for all resource management
- Prefer `std::shared_ptr` and `std::unique_ptr` for ownership
- Use `std::weak_ptr` in observer/callback scenarios to avoid cycles (especially in Wayland protocol implementations)
- Exception safety: Aim for at least basic exception safety guarantee

### Interface Abstractions

- Pure virtual interfaces (ending with `= 0`) define contracts between components
- Implementation classes typically live in `src/` while interfaces are in `include/`
- Use `override` keyword for all virtual method overrides (never omit it)

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

## Shell Development with MirAL

### Typical Shell Structure

```cpp
#include <miral/runner.h>
#include <miral/window_management_options.h>

int main(int argc, char const* argv[])
{
    using namespace miral;
    
    MirRunner runner{argc, argv};
    
    return runner.run_with({
        WindowManagementOptions{},
        DisplayConfigurationOption{},
        // Add custom policies, options, features
    });
}
```

### Window Management

- Implement `miral::WindowManagementPolicy` for custom window management
- Use `miral::WindowManagerTools` to interact with windows
- See `examples/miral-shell/` for reference implementations
- Built-in managers: `FloatingWindowManager`, `CanonicalWindowManager`, `MinimalWindowManager`

### Configuration Options

MirAL provides two mechanisms for configuration:

**`miral::ConfigurationOption`** - For command-line options and startup configuration:
- Values are set once at startup from command line, environment variables, or config file
- Use for options that don't need to change while the server is running
- Lambda callbacks can be automatically converted using `lambda_as_function`
- Options are passed to `MirRunner::run_with()` along with other initialization objects
- Example: Setting the terminal command, startup apps, or initial window management mode

**`miral::live_config::*`** - For runtime-updatable configuration:
- Values can be changed while the server is running (e.g., from a reloaded config file)
- Use for options that should respond to changes without restarting the server
- Requires a `live_config::Store` implementation (e.g., `live_config::IniFile`)
- MirAL features like `CursorScale`, `Keymap`, and `InputConfiguration` accept a Store
- Example: Accessibility settings, cursor scaling, keymaps that users can adjust on-the-fly

## Wayland Protocol Integration

When integrating a new Wayland protocol:

1. **Place protocol XML** in `wayland-protocols/` directory

2. **Generate wrapper code** using `mir_wayland_generator`:

```cmake
add_custom_command(
    OUTPUT ${PROTOCOL}_wrapper.cpp ${PROTOCOL}_wrapper.h
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${XML} header >${PROTOCOL}_wrapper.h"
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${XML} source >${PROTOCOL}_wrapper.cpp"
)
```

3. **Implement the protocol** in `src/server/frontend_wayland/`:
   - Create implementation files in the `mir::frontend` namespace
   - Implement the generated wayland protocol interfaces
   - Follow existing patterns (see `foreign_toplevel_manager_v1.cpp`, `ext_image_capture_v1.cpp`, etc.)

4. **Create a factory function** to instantiate the protocol global:
   - Expose a `create_*_manager()` or `create_*_global()` function
   - Pass required dependencies via `WaylandConnector::Context` (e.g., `MainLoop`, `Executor`, `Shell`)
   - Return `std::shared_ptr<wayland::ProtocolName::Global>`

5. **Integrate with WaylandConnector**:
   - If the protocol needs existing server components (MainLoop, Shell, etc.), access them through `WaylandConnector::Context`
   - If the protocol requires a new server component, add it to `DefaultServerConfiguration` and expose via `Server`
   - Register the protocol global with the wayland display in the appropriate initialization code

6. **Hook into MirAL** (if needed):
   - Integrate with `miral::WaylandExtensions` in your shell to enable/disable the protocol

## Linting

Pre-commit hooks (`.pre-commit-config.yaml`) handle:

- Trailing whitespace
- End-of-file fixers
- Markdown formatting (mdformat)
- Large files check

Run: `pre-commit run --all-files`

## Repository-Specific Guidelines

### Do Not Check In CodeQL Files

**Important**: Do not commit CodeQL database files or analysis results to the repository. These files are:
- Generated locally during code analysis
- Large binary files that don't belong in version control
- Automatically generated by CI/CD pipelines when needed

If you generate CodeQL databases locally for analysis, ensure they are:
- Stored outside the repository directory, or
- Properly excluded via `.gitignore`

Common CodeQL artifacts to avoid:
- CodeQL database directories (usually named `codeql-db` or similar)
- `.sarif` files containing analysis results
- Custom CodeQL query results

### Build Artifacts

Build artifacts are already excluded via `.gitignore`. Do not commit:
- Compiled binaries
- Object files
- Build directories (e.g., `build/`, `build-*/`, `cmake-*/`)
- IDE-specific files (except those already tracked)

## Additional Resources

- [Full C++ Style Guide](../doc/sphinx/contributing/reference/cppguide.md)
- [Hacking Guide](../HACKING.md)
- [Getting Started with Mir](../doc/sphinx/tutorial/getting-started-with-mir.md)

## Consistency Reminder

When in doubt, follow the style of the code you're working in. Local consistency is important for maintainability.

