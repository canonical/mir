# mir

Idiomatic Rust API for building Wayland compositors with [Mir](https://github.com/canonical/mir).

## System Requirements

This crate requires the **miral C++ library** to be installed on your system at build time and runtime.

### Ubuntu / Debian

```bash
sudo apt install libmiral-dev
```

### From Source

See the [Mir build instructions](https://github.com/canonical/mir/blob/main/HACKING.md) for building from source.

Afterwards, set your `LD_LIBRARY_PATH` to include the `miral` library's installation directory. For example:

```bash
export LD_LIBRARY_PATH=/usr/local/lib
```

## Architecture

This crate provides a safe, idiomatic Rust layer on top of the `miral` C++ library:

```
┌─────────────────────────────────────────────┐
│  mir  (this crate - idiomatic Rust API)     │
├─────────────────────────────────────────────┤
│  mir-sys  (cxx.rs FFI + bindgen)            │
├─────────────────────────────────────────────┤
│  libmiral  (C++ library, system-installed)  │
└─────────────────────────────────────────────┘
```
