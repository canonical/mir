---
applyTo: '**/CMakeLists.txt,cmake/**,**/*.cmake'
description: CMake conventions for the Mir build system.
---

# CMake conventions

- **No GLOB for sources**: All source files are listed explicitly in `add_library()` / `target_sources()`. Never use `file(GLOB ...)` for source files.
- **Test registration**: Use `mir_add_test()` and `mir_discover_tests()` from `cmake/MirCommon.cmake` — not raw `add_test()`.
- **Test binaries**: Use `mir_add_wrapped_executable(target_name NOINSTALL ...)` for test executables.
- **Wayland protocol wrappers**: Use `mir_generate_protocol_wrapper(target prefix xml_file)` — not raw `add_custom_command`.
- **Rust/CXX integration**: Use `add_rust_cxx_library()` from `cmake/RustLibrary.cmake`.
- **ABI version variables**: Each library defines `set(LIB_ABI N)` locally (e.g., `MIRAL_ABI`, `MIRCORE_ABI`, `MIRWAYLAND_ABI`, `MIRSERVER_ABI`). Bump these when breaking ABI — see the ABI & symbols instructions for the full procedure.
