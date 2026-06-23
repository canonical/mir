# mir-sys

Low-level FFI bindings to the [Mir](https://github.com/canonical/mir) library's **miral** C++ library.

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

## Usage

This crate is not intended for direct use by compositor authors. Instead, use the [`mir`](https://crates.io/crates/mir) crate which provides an idiomatic, safe Rust API.

If you need low-level access to the FFI layer:

```toml
[dependencies]
mir-sys = "0.1"
```

## How It Works

- **bindgen** generates Rust bindings for C-linkage enums from `mir_toolkit/common.h` and `mir_toolkit/events/enums.h`
- **cxx.rs** bridges C++ classes (`MirRunner`, `WindowManagerTools`, `WindowInfo`, etc.)
- **pkg-config** locates the system-installed `miral` library at build time

## License

This project is licensed under the GNU General Public License version 2 or later.
