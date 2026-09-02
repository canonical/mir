use crate::wayland_server_core::ServerState;

mod wl_fixes;

pub fn register_globals(state: &ServerState) {
    state
        .handle
        .create_global::<ServerState, wayland_server::protocol::wl_fixes::WlFixes, ()>(1u32, ());
}
