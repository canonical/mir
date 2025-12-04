# C Function Call Audit Report for `src/server`

This document provides a comprehensive audit of potentially problematic, unsafe, or deprecated C function calls in the `src/server` module of the Mir project. The audit focuses on security, error handling, resource management, and thread safety.

## Executive Summary

The Mir codebase demonstrates generally good practices with minimal use of classic unsafe C functions like `strcpy`, `sprintf`, `gets`, etc. After removing entries with adequate error handling and consolidating duplicate findings, **21 unique issues** were identified across the codebase:

1. **Environment Variable Access**: Multiple `getenv()` calls without thread-safety considerations
2. **File Operations**: Some file operations with potential TOCTOU (Time-of-Check-Time-of-Use) vulnerabilities
3. **Error Handling**: A few instances of unchecked return values
4. **Signal Handler Safety**: Use of non-async-signal-safe functions in signal handlers
5. **String Conversion**: Use of `atoi()` without proper error handling
6. **Path Truncation**: `snprintf()` calls without truncation verification

---

## Findings by Directory

### Directory: src/server (Root)

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `write` (run_mir.cpp:163, 173-183) | Return value marked as [[maybe_unused]] but not checked; multiple writes in signal handler without error handling | Check return value or handle partial writes | `while (written < len) { auto n = write(...); if (n < 0) handle_error(); written += n; }` |
| `std::strlen` (run_mir.cpp:177) | Used in signal handler; not async-signal-safe in all implementations | Use compile-time strlen or pre-calculated length | `constexpr auto name_len = /* known length */;` |
| `std::strftime` (run_mir.cpp:172) | Used in signal handler; not guaranteed async-signal-safe | Pre-format timestamp or accept limitation | `// Document limitation in comment` |
| `std::gmtime` (run_mir.cpp:172) | Used in signal handler; not async-signal-safe | Use async-signal-safe alternative or document risk | `// gmtime not async-signal-safe; acceptable risk` |

### Directory: src/server/console

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `getenv` (default_configuration.cpp:162) | Not thread-safe; called at runtime | Cache result at initialization or use thread-safe access | `static char const* cached_display = getenv("DISPLAY");` |
| `strlen` (linux_virtual_terminal.cpp:661, 663; logind_console_services.cpp:268, 792; minimal_console_services.cpp:107, 109) | Used with string literal; inefficient | Use compile-time constant | `constexpr auto prefix_len = sizeof("DEVNAME=") - 1;` |
| `strncmp` (linux_virtual_terminal.cpp:661; logind_console_services.cpp:268, 792; minimal_console_services.cpp:107) | Could use string_view or starts_with | Use modern C++ alternative | `std::string_view(line_buffer).starts_with("DEVNAME=")` |
| `::close` (logind_console_services.cpp:401, 820) | Return value not checked | Check for errors (EINTR, EIO) | `if (::close(fd_list[i]) < 0 && errno == EINTR) /* retry */;` |

### Directory: src/server/frontend_wayland

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `getenv` (wayland_connector.cpp:370) | Not thread-safe; race condition possible | Cache at initialization in a thread-safe manner | `static std::string const cached = getenv("WAYLAND_DISPLAY") ?: "";` |
| `getenv` (wayland_connector.cpp:390) | Unchecked NULL return; potential nullptr dereference | Check for NULL before use | `if (auto* dir = getenv("XDG_RUNTIME_DIR")) { chmod((std::string{dir} + "/" + wayland_display).c_str(), ...); }` |
| `chmod` (wayland_connector.cpp:390) | Potential TOCTOU vulnerability; return value not checked | Use `fchmod` with file descriptor or check return value | `if (fchmod(fd, mode) < 0) handle_error();` |
| `memcpy` (keyboard_helper.cpp:150; wl_keyboard.cpp:77; wl_pointer.cpp:80; virtual_keyboard_v1.cpp:72) | Bounds not explicitly verified in code | Add assertion or bounds check | `assert(length <= shm_buffer.size()); memcpy(...);` |
| `read` (foreign_toplevel_manager_v1.cpp:540) | Only checks for minimum size; no handling of incomplete reads | Handle short reads properly | `while (total < sizeof(inotify_event)) { ... }` |
| `strlen` (foreign_toplevel_manager_v1.cpp:584) | Used with string literal | Use compile-time constant | `constexpr auto prefix_len = sizeof(".desktop") - 1;` |

### Directory: src/server/frontend_xwayland

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `getenv` (xwayland_cursors.cpp:121; xwayland_server.cpp:91) | Not thread-safe; runtime access | Cache result at initialization | `static char const* cached = getenv("XCURSOR_SIZE");` |
| `atoi` (xwayland_cursors.cpp:125) | No error handling; returns 0 on error | Use `std::strtol` or `std::from_chars` with error checking | `char* end; long val = std::strtol(str, &end, 10); if (end != str && *end == '\0') result = val;` |
| `snprintf` (xwayland_spawner.cpp:47, 62, 144, 147, 198, 200) | Return value not checked for truncation | Verify return value is not >= buffer size | `auto n = snprintf(...); if (n >= sizeof(lockfile)) handle_truncation();` |
| `access` (xwayland_spawner.cpp:64; xwayland_connector.cpp:45) | TOCTOU vulnerability; file state can change after check | Use `open` with appropriate flags instead | `int fd = open(x11_socket, O_RDONLY); if (fd >= 0) { close(fd); /* exists */ }` |
| `unlink` (xwayland_spawner.cpp:66, 106, 199, 201) | Return value not checked | Check return value or ignore ENOENT | `if (unlink(lockfile) < 0 && errno != ENOENT) log_warning();` |

### Directory: src/server/input

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `getenv` (xkb_mapper_registrar.cpp:36, 38, 40) | Not thread-safe; multiple calls in sequence | Cache all at initialization or accept one-time initialization behavior | `static char const* loc = []{ auto l = getenv("LC_ALL"); if (!l) l = getenv("LC_CTYPE"); if (!l) l = getenv("LANG"); return l; }();` |

### Directory: src/server/report/logging

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `snprintf` (compositor_report.cpp:47, 105, 160) | Return value not checked for truncation | Verify return value | `auto n = snprintf(...); if (n >= sizeof(msg)) log_warning("truncated");` |

---

## Priority Classification

### High Priority (Security/Safety)
1. **NULL pointer dereference** (wayland_connector.cpp:390) - `getenv("XDG_RUNTIME_DIR")` can return NULL
2. **TOCTOU vulnerabilities** - `access()` followed by file operations
3. **Signal handler safety** - Non-async-signal-safe functions in signal handlers

### Medium Priority (Robustness)
1. **Unchecked return values** - `snprintf()` truncation, `unlink()`, `chmod()`
2. **Error handling** - `atoi()` without validation
3. **Thread safety** - Runtime `getenv()` calls

### Low Priority (Code Quality)
1. **Inefficient string operations** - `strlen()` on literals instead of compile-time constants
2. **Legacy string functions** - `strncmp()` instead of C++20 `starts_with()`
3. **Bounds verification** - `memcpy()` without explicit bounds assertions

---

## Recommendations

### Immediate Actions
1. Fix NULL pointer dereference in `wayland_connector.cpp:390`
2. Review and document signal handler limitations in `run_mir.cpp`
3. Add truncation checks to critical `snprintf()` calls in `xwayland_spawner.cpp`

### Short-term Improvements
1. Replace `atoi()` with `std::from_chars()` or `std::strtol()` with error checking
2. Add return value checks for `unlink()`, `chmod()`, and `close()` operations
3. Cache `getenv()` results at initialization where applicable

### Long-term Enhancements
1. Replace `access()` TOCTOU patterns with direct operation attempts
2. Modernize string operations using C++20/23 features (`std::string_view`, `starts_with()`)
3. Add compile-time `strlen` replacements using `constexpr`
4. Consider adding static analysis tools (e.g., clang-tidy, cppcheck) to CI pipeline

---

## Notes

- **Overall Code Quality**: The Mir codebase avoids most classic C pitfalls (no `strcpy`, `sprintf`, `gets`, etc.)
- **Modern C++ Usage**: Good use of RAII, smart pointers, and C++23 features
- **Error Handling**: Most critical paths have proper error handling; issues are primarily in edge cases
- **Thread Safety**: Most `getenv()` calls are in initialization contexts where thread safety is less critical

---

## Auditor Notes

This audit was performed on the `src/server` module as of the current commit. The findings represent potential issues based on static analysis and established best practices for C/C++ security and safety. Context-specific analysis may reveal that some flagged items are acceptable given their usage patterns.
