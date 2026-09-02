use crate::wayland_server_core::ServerState;
use wayland_server::{Client, DataInit, Dispatch, DisplayHandle, GlobalDispatch, New, Resource};

impl GlobalDispatch<wayland_server::protocol::wl_fixes::WlFixes, ()> for ServerState {
    fn bind(
        _state: &mut Self,
        _handle: &wayland_server::DisplayHandle,
        _client: &wayland_server::Client,
        resource: New<wayland_server::protocol::wl_fixes::WlFixes>,
        _global_data: &(),
        data_init: &mut wayland_server::DataInit<'_, Self>,
    ) {
        data_init.init(resource, ());
    }
}

impl Dispatch<wayland_server::protocol::wl_fixes::WlFixes, ()> for ServerState {
    fn request(
        _state: &mut Self,
        _client: &Client,
        _resource: &wayland_server::protocol::wl_fixes::WlFixes,
        request: <wayland_server::protocol::wl_fixes::WlFixes as wayland_server::Resource>::Request,
        _data: &(),
        dhandle: &DisplayHandle,
        _data_init: &mut DataInit<'_, Self>,
    ) {
        match request {
            wayland_server::protocol::wl_fixes::Request::DestroyRegistry { registry } => {
                let _ = dhandle
                    .backend_handle()
                    .destroy_object::<crate::wayland_server_core::ServerState>(&registry.id());
            }
            wayland_server::protocol::wl_fixes::Request::Destroy {} => {}
            _ => {}
        }
    }
}
