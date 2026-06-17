---
applyTo: tests/**
description: Testing framework, conventions, and flaky-test/WLCS guidance for Mir.
---

# Testing

## Framework

- **gtest/gmock** for unit and integration tests
- Test file naming: `test_*.cpp` (e.g., `test_floating_window_manager.cpp`)
- Mock classes: `Mock*` prefix (e.g., `MockInputDispatcher` in `mir::test::doubles`)
- Stub classes: `Stub*` or `Fake*` prefix for test doubles

## Test organization

- `tests/unit-tests/` — Component unit tests
- `tests/integration-tests/` — Cross-component integration tests
- `tests/mir_test_doubles/` — Reusable mocks and test utilities
- `mir_test_framework::WindowManagementTestHarness` — Base for window manager tests

## Example test pattern

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

## GMock matchers

Use GMock matchers with `EXPECT_THAT` and `ASSERT_THAT` for expressive assertions:

```cpp
EXPECT_THAT(value, Eq(expected));
EXPECT_THAT(result, Ne(unwanted));
EXPECT_THAT(count, Gt(0));
EXPECT_THAT(collection, Contains(item));
```

## Running tests

```sh
cd build
ctest --output-on-failure                                    # Run all tests
bin/mir_unit_tests --gtest_filter='TestSuite.test_name'      # A specific test
bin/miral-test --gtest_filter='*WindowManager*'
```

Use `mir_add_test()` / `mir_discover_tests()` and `mir_add_wrapped_executable(... NOINSTALL ...)`
from `cmake/MirCommon.cmake` to register tests — not raw `add_test()`.

## Flaky test guidelines

Intermittent test failures are tracked in [issue #4507](https://github.com/canonical/mir/issues/4507).
When writing or fixing tests:

- Use `ASSERT_*` only for preconditions and assumptions — if the assertion fails, the rest of the test is meaningless (e.g., a wait that must succeed before subsequent checks). Use `EXPECT_*` for everything else.
- Use reasonable timeouts: 1–2 seconds for callback waits is typical; 5+ seconds is excessive.
- Do not rely on tight wall-clock margins (e.g., 10ms scheduling windows) for timing-sensitive tests. Prefer controllable/fake clocks, or wait with a generous timeout and assert on measured elapsed time.
- When fixing a flaky test, address the root cause. Simply increasing a timeout is not a fix.

## WLCS (Wayland conformance) tests

- The WLCS integration lives in `tests/acceptance-tests/wayland/` — `miral_integration.cpp` builds the `miral_wlcs_integration` plugin that the external WLCS `test_runner` loads to exercise Mir. The conformance tests themselves come from the WLCS system package, not this directory.
- **`expected_wlcs_failures.list`** tracks known failures (XFAIL). When removing a protocol or fixing a bug, update this list accordingly.
- Skipped tests (e.g., protocol unavailable) that are listed as XFAIL will show as "unexpected pass" — remove them from the list.
- WLCS is a system package; version-dependent features are gated with `WLCS_VERSION` checks in CMake and `WLCS_DISPLAY_SERVER_VERSION` compile-time guards.
