/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TESTS_WAYLAND_RS_MOCK_GLOBAL_FACTORY_H
#define MIR_TESTS_WAYLAND_RS_MOCK_GLOBAL_FACTORY_H

#include "global_factory.h"

#include <gmock/gmock.h>

#include <cstdint>
#include <memory>

namespace mir
{
namespace wayland_rs
{
namespace test
{
/// A `GlobalFactory` where every method is mockable.
///
/// `GlobalFactory` has one pure-virtual factory method per global, so a test
/// interested in a single protocol would otherwise have to implement them all.
/// Use this and set expectations (or `ON_CALL(...).WillByDefault(...)`) on the
/// handful of methods the test actually cares about.
///
/// The default actions match what a server advertising nothing would do:
/// `can_view()` returns false for every interface, so a client is never told a
/// global exists and can never bind one, which in turn makes the `create_*()`
/// methods unreachable. They return null if called anyway.
///
/// Adding a new global to Mir adds a pure-virtual method here too; the compiler
/// will point at the missing mock.
class MockGlobalFactory : public GlobalFactory
{
public:
    MOCK_METHOD(bool, can_view, (rust::Str, rust::Box<WaylandClientId>), (override));

    MOCK_METHOD(std::shared_ptr<ExtDataControlManagerV1>, create_ext_data_control_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtDataControlManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtDataControlOfferV1>, create_ext_data_control_offer_v1, (rust::Box<WaylandClient>, rust::Box<ExtDataControlOfferV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtForeignToplevelListV1>, create_ext_foreign_toplevel_list_v1, (rust::Box<WaylandClient>, rust::Box<ExtForeignToplevelListV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtForeignToplevelHandleV1>, create_ext_foreign_toplevel_handle_v1, (rust::Box<WaylandClient>, rust::Box<ExtForeignToplevelHandleV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtOutputImageCaptureSourceManagerV1>, create_ext_output_image_capture_source_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtOutputImageCaptureSourceManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtForeignToplevelImageCaptureSourceManagerV1>, create_ext_foreign_toplevel_image_capture_source_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtForeignToplevelImageCaptureSourceManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtImageCopyCaptureManagerV1>, create_ext_image_copy_capture_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtImageCopyCaptureManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtInputTriggerActionManagerV1>, create_ext_input_trigger_action_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtInputTriggerActionManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtInputTriggerRegistrationManagerV1>, create_ext_input_trigger_registration_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtInputTriggerRegistrationManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ExtSessionLockManagerV1>, create_ext_session_lock_manager_v1, (rust::Box<WaylandClient>, rust::Box<ExtSessionLockManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<FractionalScaleManagerV1>, create_wp_fractional_scale_manager_v1, (rust::Box<WaylandClient>, rust::Box<FractionalScaleManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<IdleInhibitManagerV1>, create_zwp_idle_inhibit_manager_v1, (rust::Box<WaylandClient>, rust::Box<IdleInhibitManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<InputMethodContextV1>, create_zwp_input_method_context_v1, (rust::Box<WaylandClient>, rust::Box<InputMethodContextV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<InputMethodV1>, create_zwp_input_method_v1, (rust::Box<WaylandClient>, rust::Box<InputMethodV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<InputPanelV1>, create_zwp_input_panel_v1, (rust::Box<WaylandClient>, rust::Box<InputPanelV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<InputMethodManagerV2>, create_zwp_input_method_manager_v2, (rust::Box<WaylandClient>, rust::Box<InputMethodManagerV2Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<LinuxDmabufV1>, create_zwp_linux_dmabuf_v1, (rust::Box<WaylandClient>, rust::Box<LinuxDmabufV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<LinuxDrmSyncobjManagerV1>, create_wp_linux_drm_syncobj_manager_v1, (rust::Box<WaylandClient>, rust::Box<LinuxDrmSyncobjManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<MirShellV1>, create_mir_shell_v1, (rust::Box<WaylandClient>, rust::Box<MirShellV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<PointerConstraintsV1>, create_zwp_pointer_constraints_v1, (rust::Box<WaylandClient>, rust::Box<PointerConstraintsV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<PrimarySelectionDeviceManagerV1>, create_zwp_primary_selection_device_manager_v1, (rust::Box<WaylandClient>, rust::Box<PrimarySelectionDeviceManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<PrimarySelectionOfferV1>, create_zwp_primary_selection_offer_v1, (rust::Box<WaylandClient>, rust::Box<PrimarySelectionOfferV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<RelativePointerManagerV1>, create_zwp_relative_pointer_manager_v1, (rust::Box<WaylandClient>, rust::Box<RelativePointerManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ServerDecorationManager>, create_org_kde_kwin_server_decoration_manager, (rust::Box<WaylandClient>, rust::Box<ServerDecorationManagerMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<TextInputManagerV1>, create_zwp_text_input_manager_v1, (rust::Box<WaylandClient>, rust::Box<TextInputManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<TextInputManagerV2>, create_zwp_text_input_manager_v2, (rust::Box<WaylandClient>, rust::Box<TextInputManagerV2Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<TextInputManagerV3>, create_zwp_text_input_manager_v3, (rust::Box<WaylandClient>, rust::Box<TextInputManagerV3Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<Viewporter>, create_wp_viewporter, (rust::Box<WaylandClient>, rust::Box<ViewporterMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<VirtualKeyboardManagerV1>, create_zwp_virtual_keyboard_manager_v1, (rust::Box<WaylandClient>, rust::Box<VirtualKeyboardManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<Compositor>, create_wl_compositor, (rust::Box<WaylandClient>, rust::Box<CompositorMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<Shm>, create_wl_shm, (rust::Box<WaylandClient>, rust::Box<ShmMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<DataOffer>, create_wl_data_offer, (rust::Box<WaylandClient>, rust::Box<DataOfferMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<DataDeviceManager>, create_wl_data_device_manager, (rust::Box<WaylandClient>, rust::Box<DataDeviceManagerMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<Shell>, create_wl_shell, (rust::Box<WaylandClient>, rust::Box<ShellMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<Seat>, create_wl_seat, (rust::Box<WaylandClient>, rust::Box<SeatMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<Subcompositor>, create_wl_subcompositor, (rust::Box<WaylandClient>, rust::Box<SubcompositorMiddleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ForeignToplevelManagerV1>, create_zwlr_foreign_toplevel_manager_v1, (rust::Box<WaylandClient>, rust::Box<ForeignToplevelManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ForeignToplevelHandleV1>, create_zwlr_foreign_toplevel_handle_v1, (rust::Box<WaylandClient>, rust::Box<ForeignToplevelHandleV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<LayerShellV1>, create_zwlr_layer_shell_v1, (rust::Box<WaylandClient>, rust::Box<LayerShellV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<ScreencopyManagerV1>, create_zwlr_screencopy_manager_v1, (rust::Box<WaylandClient>, rust::Box<ScreencopyManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<VirtualPointerManagerV1>, create_zwlr_virtual_pointer_manager_v1, (rust::Box<WaylandClient>, rust::Box<VirtualPointerManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<XdgActivationV1>, create_xdg_activation_v1, (rust::Box<WaylandClient>, rust::Box<XdgActivationV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<XdgDecorationManagerV1>, create_zxdg_decoration_manager_v1, (rust::Box<WaylandClient>, rust::Box<XdgDecorationManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<XdgWmDialogV1>, create_xdg_wm_dialog_v1, (rust::Box<WaylandClient>, rust::Box<XdgWmDialogV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<XdgOutputManagerV1>, create_zxdg_output_manager_v1, (rust::Box<WaylandClient>, rust::Box<XdgOutputManagerV1Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<XdgShellV6>, create_zxdg_shell_v6, (rust::Box<WaylandClient>, rust::Box<XdgShellV6Middleware>, uint32_t), (override));
    MOCK_METHOD(std::shared_ptr<XdgWmBase>, create_xdg_wm_base, (rust::Box<WaylandClient>, rust::Box<XdgWmBaseMiddleware>, uint32_t), (override));
};
}
}
}

#endif  // MIR_TESTS_WAYLAND_RS_MOCK_GLOBAL_FACTORY_H
