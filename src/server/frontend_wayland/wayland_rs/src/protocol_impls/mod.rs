use crate::wayland_server_core::ServerState;

mod wl_fixes;

pub fn register_globals(state: &ServerState) {
    state
        .handle
        .create_global::<ServerState, crate::protocols::wl_fixes::wl_fixes::WlFixes, ()>(1u32, ());
}
