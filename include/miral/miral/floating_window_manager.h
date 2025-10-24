/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIRAL_FLOATING_WINDOW_MANAGER_H
#define MIRAL_FLOATING_WINDOW_MANAGER_H

#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>
#include <mir_toolkit/events/enums.h>

namespace miral
{
/// A minimal floating window management policy.
///
/// This policy provides the following features:
///
/// - A stacking, floating window management policy
/// - The ability to move and resize windows
/// - Touchscreen support
/// - Alt + F4 keybind to close windows
/// - Focus stealing management
///
/// Compositor authors may elect to use this class as provided or subclass it in order
/// to extend the base functionality.
///
/// This class is the successor to #miral::MinimalWindowManager.
///
/// \remark Since MirAL 5.6
class FloatingWindowManager : public WindowManagementPolicy
{
public:
    explicit FloatingWindowManager(WindowManagerTools const& tools);
    FloatingWindowManager(WindowManagerTools const& tools, FocusStealing focus_stealing, MirInputEventModifier pointer_drag_modifier);
    ~FloatingWindowManager() override;

    auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification override;

    /// Focuses the window if it can receive focus.
    ///
    /// \param window_info the ready window
    void handle_window_ready(WindowInfo& window_info) override;

    /// Honors the modifications requested by the client.
    ///
    /// \param window_info the window being modified
    /// \param modifications the modifications to the window
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    /// Gives focus to the window being raised.
    ///
    /// \param window_info the window being raised
    void handle_raise_window(WindowInfo& window_info) override;

    /// Honors the requested placement of the window.
    ///
    /// \param window_info the window being placed
    /// \param new_state the new state of the window
    /// \param new_placement the new rectangle of the window
    auto confirm_placement_on_display(
        WindowInfo const& window_info, MirWindowState new_state, Rectangle const& new_placement) -> Rectangle override;

    /// Handles Alt-F4 to close the active window.
    ///
    /// \param event keyboard event
    /// \returns `true` if the event was handled, otherwise `false`
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    /// Handles touch to focus, resizing wndows with touch, and moving windows with touch.
    ///
    /// \param event touch event
    /// \returns `true` if the event was handled, otherwise `false`
    bool handle_touch_event(MirTouchEvent const* event) override;

    /// Handles focus selection, resizing windows with the pointer, and moving windows with the pointer.
    ///
    /// \param event pointer events
    /// \returns `true` if the event was handled, otherwise `false`
    bool handle_pointer_event(MirPointerEvent const* event) override;

    /// Initiates a move gesture from the client.
    ///
    /// This is only implemented for pointers.
    ///
    /// \param window_info the window being moved
    /// \param input_event the input event that caused the movement
    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;

    /// Initiates a resize gesture from the client.
    ///
    /// This is only implemented for pointers.
    ///
    /// \param window_info the window being resized
    /// \param input_event the input event that causes the resize
    /// \param edge the edge from which the resize happens
    void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

    /// Honors the requested inherited move.
    ///
    /// \param window_info the window being moved
    /// \param movement the displacement of the window
    auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle override;

    /// Raises the provided window on focus.
    ///
    /// \param window_info the window receiving focus
    void advise_focus_gained(WindowInfo const& window_info) override;

    /// Raises the window based on the #miral::FocusStealing of the policy.
    ///
    /// \param window_info the new window
    void advise_new_window(WindowInfo const& window_info) override;

protected:
    WindowManagerTools tools;

    bool begin_pointer_move(WindowInfo const& window_info, MirInputEvent const* input_event);
    bool begin_pointer_resize(WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge);

    bool begin_touch_move(WindowInfo const& window_info, MirInputEvent const* input_event);
    bool begin_touch_resize(WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge);

private:
    struct Impl;
    std::unique_ptr<Impl> const self;
};
}

#endif //MIRAL_FLOATING_WINDOW_MANAGER_H
