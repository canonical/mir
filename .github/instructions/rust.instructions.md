---
applyTo: src/platforms/evdev-rs/**
description: Rust / CXX safety standards for the evdev-rs input platform.
---

# Rust components

The `evdev-rs` input platform (`src/platforms/evdev-rs/`) is written in Rust and bridges to
C++ via [CXX](https://cxx.rs/).

## Safety standards

- All `unsafe` blocks must have a `// # Safety` comment explaining **why** the invariants are upheld, not just restating what is unsafe. Bad: "This is unsafe because it receives a raw C++ pointer." Good: "The C++ caller guarantees `platform` outlives `self` and is only accessed from the event loop thread."
- Enable the `undocumented_unsafe_blocks` clippy lint — see [issue #4841](https://github.com/canonical/mir/issues/4841).
- The CXX bridge in `src/platforms/evdev-rs/src/ffi.rs` defines the Rust/C++ FFI boundary. Follow existing patterns when adding new FFI functions.
- Integrate Rust libraries into the build with `add_rust_cxx_library()` from `cmake/RustLibrary.cmake`.
