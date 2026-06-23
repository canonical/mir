# mir-sys

Low-level FFI bindings for [Mir](https://github.com/canonical/mir).

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

This crate is not intended for direct use by compositor authors. Instead, use the `mir` crate at the project root which provides an idiomatic, safe Rust API.

## License

This project is licensed under the GNU General Public License version 2 or later.
