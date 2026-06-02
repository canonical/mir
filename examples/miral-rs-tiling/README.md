# miral-rs-tiling

A minimal tiling window manager example using the [miral](../../miral-rs/) Rust API for [Mir](https://github.com/canonical/mir).

This example demonstrates:

- Implementing the `WindowManagementPolicy` trait
- Using the `Advice` enum for lifecycle notifications
- External storage pattern for per-application data
- Builder-pattern `MirRunner` configuration

Windows are tiled in a simple horizontal split layout — each new window gets an equal fraction of the available screen width.

## System Requirements

The `miral` crate requires the **miral C++ library** to be installed on your system.

### Ubuntu / Debian

```bash
sudo apt install libmiral-dev
```

### From Source

See the [Mir build instructions](https://github.com/canonical/mir/blob/main/HACKING.md) for building from source. Afterwards, ensure the library is discoverable:

```bash
export LD_LIBRARY_PATH=/usr/local/lib
```

## Running

From the repository root:

```bash
WAYLAND_DISPLAY=wayland-98 cargo run -p miral-rs-tiling
```

Or from this directory:

```bash
WAYLAND_DISPLAY=wayland-98 cargo run
```

The compositor will start and tile any opened windows horizontally.

## Keybindings

| Shortcut    | Action                    |
| ----------- | ------------------------- |
| `Alt+Enter` | Launch terminal (konsole) |
| `Alt+Q`     | Close focused window      |
