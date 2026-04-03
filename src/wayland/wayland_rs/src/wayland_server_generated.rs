use crate::ffi::GlobalFactory;
use cxx::UniquePtr;
use std::sync::{Mutex, Arc};
unsafe impl Send for GlobalFactory {}
unsafe impl Sync for GlobalFactory {}
impl WaylandServer {
    pub fn register_globals(
        state: &ServerState,
        factory: Arc<Mutex<UniquePtr<GlobalFactory>>>,
    ) {
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_data_control_v1::ext_data_control_manager_v1::ExtDataControlManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_data_control_v1::ext_data_control_offer_v1::ExtDataControlOfferV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_foreign_toplevel_list_v1::ext_foreign_toplevel_list_v1::ExtForeignToplevelListV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_foreign_toplevel_list_v1::ext_foreign_toplevel_handle_v1::ExtForeignToplevelHandleV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_image_capture_source_v1::ext_output_image_capture_source_manager_v1::ExtOutputImageCaptureSourceManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_image_capture_source_v1::ext_foreign_toplevel_image_capture_source_manager_v1::ExtForeignToplevelImageCaptureSourceManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_image_copy_capture_v1::ext_image_copy_capture_manager_v1::ExtImageCopyCaptureManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_input_trigger_action_v1::ext_input_trigger_action_manager_v1::ExtInputTriggerActionManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_input_trigger_registration_v1::ext_input_trigger_registration_manager_v1::ExtInputTriggerRegistrationManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::ext_session_lock_v1::ext_session_lock_manager_v1::ExtSessionLockManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::fractional_scale_v1::wp_fractional_scale_manager_v1::WpFractionalScaleManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::idle_inhibit_unstable_v1::zwp_idle_inhibit_manager_v1::ZwpIdleInhibitManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::input_method_unstable_v1::zwp_input_method_context_v1::ZwpInputMethodContextV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::input_method_unstable_v1::zwp_input_method_v1::ZwpInputMethodV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::input_method_unstable_v1::zwp_input_panel_v1::ZwpInputPanelV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::input_method_unstable_v2::zwp_input_method_manager_v2::ZwpInputMethodManagerV2,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::linux_dmabuf_v1::zwp_linux_dmabuf_v1::ZwpLinuxDmabufV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(5u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::linux_drm_syncobj_v1::wp_linux_drm_syncobj_manager_v1::WpLinuxDrmSyncobjManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::mir_shell_unstable_v1::mir_shell_v1::MirShellV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::pointer_constraints_unstable_v1::zwp_pointer_constraints_v1::ZwpPointerConstraintsV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wp_primary_selection_unstable_v1::zwp_primary_selection_device_manager_v1::ZwpPrimarySelectionDeviceManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wp_primary_selection_unstable_v1::zwp_primary_selection_offer_v1::ZwpPrimarySelectionOfferV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::relative_pointer_unstable_v1::zwp_relative_pointer_manager_v1::ZwpRelativePointerManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::server_decoration::org_kde_kwin_server_decoration_manager::OrgKdeKwinServerDecorationManager,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::text_input_unstable_v1::zwp_text_input_manager_v1::ZwpTextInputManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::text_input_unstable_v2::zwp_text_input_manager_v2::ZwpTextInputManagerV2,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::text_input_unstable_v3::zwp_text_input_manager_v3::ZwpTextInputManagerV3,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::viewporter::wp_viewporter::WpViewporter,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::virtual_keyboard_unstable_v1::zwp_virtual_keyboard_manager_v1::ZwpVirtualKeyboardManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_compositor::WlCompositor,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(6u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_shm::WlShm,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_data_offer::WlDataOffer,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(3u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_data_device_manager::WlDataDeviceManager,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(3u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_shell::WlShell,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_seat::WlSeat,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(9u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_output::WlOutput,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(4u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                wayland_server::protocol::wl_subcompositor::WlSubcompositor,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wlr_foreign_toplevel_management_unstable_v1::zwlr_foreign_toplevel_manager_v1::ZwlrForeignToplevelManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(2u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wlr_foreign_toplevel_management_unstable_v1::zwlr_foreign_toplevel_handle_v1::ZwlrForeignToplevelHandleV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(2u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wlr_layer_shell_unstable_v1::zwlr_layer_shell_v1::ZwlrLayerShellV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(4u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wlr_screencopy_unstable_v1::zwlr_screencopy_manager_v1::ZwlrScreencopyManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(3u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::wlr_virtual_pointer_unstable_v1::zwlr_virtual_pointer_manager_v1::ZwlrVirtualPointerManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(2u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::xdg_activation_v1::xdg_activation_v1::XdgActivationV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::xdg_decoration_unstable_v1::zxdg_decoration_manager_v1::ZxdgDecorationManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::xdg_output_unstable_v1::zxdg_output_manager_v1::ZxdgOutputManagerV1,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(3u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::xdg_shell_unstable_v6::zxdg_shell_v6::ZxdgShellV6,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(1u32, factory.clone());
        state
            .handle
            .create_global::<
                ServerState,
                protocols::xdg_shell::xdg_wm_base::XdgWmBase,
                Arc<Mutex<UniquePtr<GlobalFactory>>>,
            >(5u32, factory.clone());
    }
}
