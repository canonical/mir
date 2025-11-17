# Mir Compositor Coding Instructions

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

## Build System

### Building

```bash
mkdir build && cd build
cmake ..
make
```

### Speed optimizations

```bash
cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DMIR_USE_PRECOMPILED_HEADERS=OFF \
      -DMIR_USE_LD=mold ..
```

### Key CMake patterns

- `mir_add_wrapped_executable()`: Creates executables with wrapper scripts
- Compile commands available at `build/compile_commands.json` for LSP
- Uses C++23 standard (`CMAKE_CXX_STANDARD=23`)
- Fatal warnings enabled by default (`MIR_FATAL_COMPILE_WARNINGS=ON`)

## Code Conventions

### Error Handling

- Functions throw exceptions if they cannot meet post-conditions (minimum: basic exception safety)
- Preconditions are NOT checked - use `assert` for precondition verification
- Pointers are assumed to refer to valid objects unless explicitly documented otherwise

### Namespaces

- `mir::` - root namespace
- `mir::geometry::` - geometric types (Point, Size, Rectangle, etc.)
- `mir::graphics::`, `mir::input::`, `mir::scene::` - component namespaces
- `miral::` - Abstraction layer APIs
- `namespace geom = mir::geometry;` - common alias in implementation files

### Header Organization

```cpp
// Public headers (include/) - interfaces only, no platform types
// Implementation headers (src/) - platform-specific, within component only
```

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

## Shell Development with mirAL

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

## Wayland Protocol Integration

1. Place protocol XML in `wayland-protocols/`
2. Generate wrappers using `mir_wayland_generator`:

```cmake
add_custom_command(
    OUTPUT ${PROTOCOL}_wrapper.cpp ${PROTOCOL}_wrapper.h
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${XML} header >${PROTOCOL}_wrapper.h"
    COMMAND "sh" "-c" "mir_wayland_generator zwp_ ${XML} source >${PROTOCOL}_wrapper.cpp"
)
```

3. Integrate with `miral::WaylandExtensions` in your shell

## Debugging

### Component Reports

Enable runtime debugging via environment or command-line options (see `doc/sphinx/explanation/component_reports.md`)

### Mesa/GBM Debugging

Build debug Mesa packages for easier graphics stack debugging:

```bash
apt source mesa
# Edit debian/rules: change -Db_ndebug=true to =false
DEB_BUILD_OPTIONS="noopt nostrip" sbuild -d $UBUNTU_CODENAME -j 32
sudo dpkg --install --force-overwrite ../*.deb
```

Then in gdb: `directory $MESA_BUILD_DIR/build`

## Linting

Pre-commit hooks (`.pre-commit-config.yaml`) handle:

- Trailing whitespace
- End-of-file fixers
- Markdown formatting (mdformat)
- Large files check

Run: `pre-commit run --all-files`
