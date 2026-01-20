use crate::wayland_server::{create_wayland_server, WaylandServer};

#[cxx::bridge(namespace = "mir::wayland_rs")]
mod ffi_rust {
    extern "Rust" {
        type WaylandServer;

        fn create_wayland_server() -> Box<WaylandServer>;
        fn run(self: &mut WaylandServer, socket: &str);
    }
}
