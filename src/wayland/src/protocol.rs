// This section is where we generate the protocols. Unfortunately, we are at least
// forces to use the internal wayland protocols from the wayland-server crate. We
// may generate the rest ourselves however.
//
// https://docs.rs/wayland-scanner/latest/wayland_scanner/
use wayland_server;
use wayland_server::protocol::*;
use wayland_server::protocol::__interfaces::*;

pub mod xdg_shell_unstable_v6 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/xdg-shell-unstable-v6.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/xdg-shell-unstable-v6.xml");
}

pub mod xdg_shell {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/xdg-shell.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/xdg-shell.xml");
}

pub mod xdg_output_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/xdg-output-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/xdg-output-unstable-v1.xml");
}

pub mod wlr_layer_shell_unstable_v1 {
    use crate::protocol::xdg_shell::*;
    wayland_scanner::generate_interfaces!("../../wayland-protocols/wlr-layer-shell-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/wlr-layer-shell-unstable-v1.xml");
}

pub mod wlr_foreign_toplevel_management_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/wlr-foreign-toplevel-management-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/wlr-foreign-toplevel-management-unstable-v1.xml");
}

pub mod pointer_constraints_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/pointer-constraints-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/pointer-constraints-unstable-v1.xml");
}

pub mod relative_pointer_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/relative-pointer-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/relative-pointer-unstable-v1.xml");
}

pub mod virtual_keyboard_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/virtual-keyboard-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/virtual-keyboard-unstable-v1.xml");
}

pub mod text_input_unstable_v3 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/text-input-unstable-v3.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/text-input-unstable-v3.xml");
}

pub mod text_input_unstable_v2 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/text-input-unstable-v2.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/text-input-unstable-v2.xml");
}

pub mod text_input_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/text-input-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/text-input-unstable-v1.xml");
}

pub mod input_method_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/input-method-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/input-method-unstable-v1.xml");
}

pub mod input_method_unstable_v2 {
    use crate::protocol::text_input_unstable_v3::*;
    wayland_scanner::generate_interfaces!("../../wayland-protocols/input-method-unstable-v2.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/input-method-unstable-v2.xml");
}

pub mod idle_inhibit_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/idle-inhibit-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/idle-inhibit-unstable-v1.xml");
}

pub mod primary_selection_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/primary-selection-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/primary-selection-unstable-v1.xml");
}

pub mod wlr_screencopy_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/wlr-screencopy-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/wlr-screencopy-unstable-v1.xml");
}

pub mod wlr_virtual_pointer_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/wlr-virtual-pointer-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/wlr-virtual-pointer-unstable-v1.xml");
}

pub mod ext_session_lock_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/ext-session-lock-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/ext-session-lock-v1.xml");
}

pub mod mir_shell_unstable_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/mir-shell-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/mir-shell-unstable-v1.xml");
}

pub mod xdg_decoration_unstable_v1 {
    use crate::protocol::xdg_shell::XDG_TOPLEVEL_INTERFACE;
    use crate::protocol::xdg_shell::xdg_toplevel_interface;
    use crate::protocol::xdg_shell::xdg_toplevel;
    wayland_scanner::generate_interfaces!("../../wayland-protocols/xdg-decoration-unstable-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/xdg-decoration-unstable-v1.xml");
}

pub mod viewporter {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/viewporter.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/viewporter.xml");
}

pub mod fractional_scale_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/fractional-scale-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/fractional-scale-v1.xml");
}

pub mod xdg_activation_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/xdg-activation-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/xdg-activation-v1.xml");
}

pub mod linux_drm_syncobj_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/linux-drm-syncobj-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/linux-drm-syncobj-v1.xml");
}

pub mod ext_data_control_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/ext-data-control-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/ext-data-control-v1.xml");
}

pub mod ext_foreign_toplevel_list_v1 {
    wayland_scanner::generate_interfaces!("../../wayland-protocols/ext-foreign-toplevel-list-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/ext-foreign-toplevel-list-v1.xml");
}

pub mod ext_image_capture_source_v1 {
    use crate::protocol::ext_foreign_toplevel_list_v1::EXT_FOREIGN_TOPLEVEL_HANDLE_V1_INTERFACE;
    use crate::protocol::ext_foreign_toplevel_list_v1::ext_foreign_toplevel_handle_v1_interface;
    use crate::protocol::ext_foreign_toplevel_list_v1::ext_foreign_toplevel_handle_v1;
    wayland_scanner::generate_interfaces!("../../wayland-protocols/ext-image-capture-source-v1.xml");
    use super::*;
    wayland_scanner::generate_server_code!("../../wayland-protocols/ext-image-capture-source-v1.xml");
}
