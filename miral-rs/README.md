# miral

Idiomatic Rust API for building Wayland compositors with the [Mir](https://github.com/canonical/mir) display server.

## System Requirements

This crate requires the **miral C++ library** to be installed on your system at build time and runtime.

### Ubuntu / Debian

```bash
sudo apt install libmiral-dev
```

### From Source

See the [Mir build instructions](https://github.com/canonical/mir/blob/main/HACKING.md) for building from source.

## Quick Start

```rust
use miral::prelude::*;

struct MyPolicy {
    tools: WindowManagerTools,
}

impl WindowManagementPolicy for MyPolicy {
    fn tools(&self) -> &WindowManagerTools { &self.tools }

    fn handle_keyboard_event(&mut self, event: &KeyboardEvent) -> bool {
        // Handle keyboard shortcuts here
        false
    }

    fn advise(&mut self, event: Advice) {
        match event {
            Advice::NewWindow { window_info } => {
                println!("New window: {:?}", window_info.name());
            }
            _ => {}
        }
    }
}

fn main() {
    MirRunner::new(std::env::args())
        .add(WaylandExtensions::default())
        .add(Decorations::prefer_csd())
        .add_window_management_policy::<MyPolicy>()
        .run()
        .expect("Server failed");
}
```

## Architecture

This crate provides a safe, idiomatic Rust layer on top of the battle-tested miral C++ library:

```
┌─────────────────────────────────────────────┐
│  miral  (this crate - idiomatic Rust API)   │
├─────────────────────────────────────────────┤
│  miral-sys  (cxx.rs FFI + bindgen)          │
├─────────────────────────────────────────────┤
│  libmiral  (C++ library, system-installed)  │
└─────────────────────────────────────────────┘
```

## Key Concepts

- **`MirRunner`** — Manages the compositor lifecycle (builder pattern)
- **`WindowManagementPolicy`** — Trait for custom window management logic
- **`WindowManagerTools`** — API for querying and modifying windows
- **`Advice`** — Enum of lifecycle notifications (new window, focus change, etc.)
- **`WindowSpecification`** — Builder for window property changes

## License

This project is licensed under the GNU General Public License version 2 or later.
