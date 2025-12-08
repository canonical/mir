# Code Audit Report: C Function Calls in src/platforms

**Generated:** 2025-12-04  
**Auditor:** Copilot Security Audit Agent  
**Scope:** Security and safety audit of C function calls in the Mir platform code (`src/platforms`)  
**Analysis Criteria:** Safety/Security, Error Handling, Resource Management, Performance

---

## Executive Summary

This audit examined 83 C/C++ source files across 10 subdirectories in the `src/platforms` module. The analysis focused on identifying potentially unsafe C function calls including buffer operations, memory management, file I/O, and system calls.

**Key Findings:**
- **Critical Issues:** 2 (EINTR handling, file descriptor management)
- **Medium Priority:** 5 (RAII improvements, resource management)
- **Low Priority:** 2 (code style improvements)

---

## Directory: src/platforms/atomic-kms/server/kms

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `free` (display.cpp:474) | Manual memory management of C string from DRM library | Use RAII wrapper (std::unique_ptr with custom deleter) | `std::unique_ptr<char[], CStrFree> str{drmGetPrimaryDeviceNameFromFd(fd)};` |

## Directory: src/platforms/common/server/kms-utils

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `read` (threaded_drm_event_handler.cpp:50) | **CRITICAL:** read() may be interrupted by signals (EINTR not handled) | Wrap in loop to handle EINTR | `while ((ret = ::read(read, &buffer, sizeof(buffer))) != sizeof(buffer) && errno == EINTR);` |
| `write` (threaded_drm_event_handler.cpp:59) | **CRITICAL:** write() may be interrupted by signals (EINTR not handled) | Wrap in loop to handle EINTR | `while ((ret = ::write(write, &data, sizeof(data))) != sizeof(data) && errno == EINTR);` |
| `memset` (threaded_drm_event_handler.cpp:163) | Clearing struct - acceptable pattern for C structures | Consider using value initialization | `drmEventContext ctx{};` |

## Directory: src/platforms/gbm-kms/server/kms

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `free` (display.cpp:507) | Manual memory management of DRM C string | Use RAII wrapper | `std::unique_ptr<char[], CStrFree> str{...};` |
| `memset` (display_buffer.cpp:90) | Constant value (24) used - seems questionable | Should be 0 for clearing? Verify intent | `::memset(mapping->data(), 0, mapping->len());` |
| `memset` (kms_page_flipper.cpp:92) | Clearing C struct | Use value initialization | `drmEventContext evctx{};` |

## Directory: src/platforms/wayland

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `free` (cursor.cpp:48) | Manual free after strdup - mixing malloc/free with C++ | Use std::string or RAII wrapper | `auto filename = std::make_unique<char[]>(...); or use std::string` |
| `close` (displayclient.cpp:608) | **CRITICAL:** Direct close() instead of RAII | Use mir::Fd or unique_fd | `mir::Fd fd{...}; // Auto-closes on destruction` |

## Directory: src/platforms/x11

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `free` (x11_resources.cpp:102) | Manual free of XCB reply | Use unique_ptr with custom deleter | `std::unique_ptr<xcb_intern_atom_reply_t, decltype(&free)> reply{xcb_intern_atom_reply(...), &free};` |

## Directory: src/platforms/x11/graphics

| Function Name / Call Site | Issue | Suggested Fix | Code Fix |
| :--- | :--- | :--- | :--- |
| `free` (egl_helper.cpp:178) | Manual free of XCB reply | Use unique_ptr with custom deleter | `std::unique_ptr<xcb_get_geometry_reply_t, decltype(&free)> reply{xcb_get_geometry_reply(...), &free};` |

---

## Detailed Analysis

### Critical Issues

#### 1. EINTR Handling in threaded_drm_event_handler.cpp
**Severity:** High  
**Files:** `common/server/kms-utils/threaded_drm_event_handler.cpp`

The `read()` (line 50) and `write()` (line 59) system calls can be interrupted by signals, returning with `errno` set to `EINTR`. The current implementation does not handle this case, which could lead to spurious errors in signal-heavy environments.

**Recommendation:** Wrap these calls in a loop that retries on EINTR:
```cpp
ssize_t ret;
while ((ret = ::read(read, &buffer, sizeof(buffer))) != sizeof(buffer))
{
    if (errno != EINTR)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to read from pipe"}));
    }
}
```

#### 2. File Descriptor Leak Risk in wayland/displayclient.cpp
**Severity:** High  
**Files:** `wayland/displayclient.cpp:608`

Direct call to `close()` instead of using RAII could lead to file descriptor leaks if an exception is thrown before the close call.

**Recommendation:** Use `mir::Fd` RAII wrapper throughout the function.

### Medium Priority Issues

#### 3. Manual Memory Management
**Severity:** Medium  
**Files:** 
- `atomic-kms/server/kms/display.cpp:474`
- `gbm-kms/server/kms/display.cpp:507`
- `wayland/cursor.cpp:48`
- `x11/x11_resources.cpp:102`
- `x11/graphics/egl_helper.cpp:178`

Multiple instances of manual `free()` calls for C library allocations. While these are not necessarily bugs (if exception-safe), they violate modern C++ best practices.

**Recommendation:** Use `std::unique_ptr` with custom deleters for RAII:
```cpp
std::unique_ptr<char[], decltype(&free)> str{drmGetPrimaryDeviceNameFromFd(fd), &free};
```

#### 4. memset with Suspicious Value
**Severity:** Medium  
**Files:** `gbm-kms/server/kms/display_buffer.cpp:90`

Uses `memset(mapping->data(), 24, mapping->len())` which sets every byte to 24 (0x18). This seems unusual for buffer initialization.

**Recommendation:** Verify if this is intentional. If clearing is intended, use 0.

### Low Priority Issues

#### 5. Struct Initialization Style
**Severity:** Low  
**Files:** 
- `threaded_drm_event_handler.cpp:163`
- `kms_page_flipper.cpp:92`

Uses `memset(&ctx, 0, sizeof(ctx))` for C struct initialization.

**Recommendation:** Use C++11 value initialization: `drmEventContext ctx{};`

### Safe Patterns Verified (Not Included Above)

The following patterns were audited and verified as safe, and are therefore not included in the issue tables above:

✅ **All `open()` calls** are properly wrapped in `mir::Fd` RAII wrapper  
✅ **All `memcpy()` operations** have verified bounds calculations  
✅ **All `sscanf()` calls** check return values  
✅ **DRM library error handling** properly manages errno  
✅ **Buffer operations in cursor.cpp** have proper size calculations

---

## Recommendations Priority

1. **High Priority:** Fix EINTR handling in threaded_drm_event_handler.cpp
2. **High Priority:** Convert close() to RAII in wayland/displayclient.cpp
3. **Medium Priority:** Convert manual free() calls to RAII wrappers
4. **Medium Priority:** Verify memset value in display_buffer.cpp
5. **Low Priority:** Modernize struct initialization to use value initialization

---

## Files Examined

Total files scanned: 83 C/C++ source files

### Subdirectories:
- atomic-kms (6 files)
- common (15 files)
- eglstream-kms (12 files)
- evdev (8 files)
- evdev-rs (3 files)
- gbm-kms (18 files)
- renderer-generic-egl (2 files)
- virtual (4 files)
- wayland (9 files)
- x11 (6 files)

---

## Methodology

1. **Pattern Matching:** Automated scanning for known unsafe function patterns
2. **Context Analysis:** Manual review of each identified call site
3. **Bounds Verification:** Verification of buffer size calculations
4. **Error Handling Review:** Checking of return value handling
5. **RAII Compliance:** Assessment of resource management patterns

---

## Conclusion

The `src/platforms` code generally follows good practices with extensive use of RAII wrappers and proper error handling. The issues identified above are specific, localized, and can be fixed without major architectural changes. Many flagged instances during initial scanning were determined to be false positives where proper bounds checking or error handling was already in place - these have been excluded from this report and documented in the "Safe Patterns Verified" section.
