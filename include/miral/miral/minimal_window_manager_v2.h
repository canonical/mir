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

#ifndef MIRAL_MINIMAL_WINDOW_MANAGER_V2_H
#define MIRAL_MINIMAL_WINDOW_MANAGER_V2_H

#include "focus_stealing.h"
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
class MinimalWindowManagerV2 : public WindowManagementPolicy
{
public:
    explicit MinimalWindowManagerV2(WindowManagerTools const& tools);
    MinimalWindowManagerV2& focus_stealing(FocusStealing focus_stealing);
    MinimalWindowManagerV2& pointer_drag_modifier(MirInputEventModifier pointer_drag_modifier);

    auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification override;

    void handle_window_ready(WindowInfo& window_info) override;
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;
    void handle_raise_window(WindowInfo& window_info) override;
    auto confirm_placement_on_display(
        WindowInfo const& window_info, MirWindowState new_state, Rectangle const& new_placement) -> Rectangle override;
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    bool handle_touch_event(MirTouchEvent const* event) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;
    void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;
    auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle override;
    void advise_focus_gained(WindowInfo const& window_info) override;
    void advise_new_app(ApplicationInfo& app_info) override;
    void advise_delete_app(ApplicationInfo const& app_info) override;
    void advise_new_window(WindowInfo const& app_info) override;

protected:
    WindowManagerTools tools;

    bool begin_pointer_move(WindowInfo const& window_info, MirInputEvent const* input_event);
    bool begin_pointer_resize(WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge);

    bool begin_touch_move(WindowInfo const& window_info, MirInputEvent const* input_event);
    bool begin_touch_resize(WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge);

private:
    struct Impl;
    std::shared_ptr<Impl> const self;
};
}

#endif //MIRAL_MINIMAL_WINDOW_MANAGER_V2_H
