/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_MINIMAL_WINDOW_MANAGER_H
#define MIRAL_MINIMAL_WINDOW_MANAGER_H

#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>

namespace miral
{
/// Minimal implementation of a floating window management policy
/// \remark Since MirAL 2.5
class MinimalWindowManager : public WindowManagementPolicy
{
public:
    explicit MinimalWindowManager(WindowManagerTools const& tools);
    ~MinimalWindowManager();

    /// Honours the requested specification
    auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification override;

    /// If the window can have focus it is given focus
    void handle_window_ready(WindowInfo& window_info) override;

    /// Honours the requested modifications
    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    /// Gives focus to the requesting window (tree)
    void handle_raise_window(WindowInfo& window_info) override;

    /// Honours the requested placement
    auto confirm_placement_on_display(
        WindowInfo const& window_info, MirWindowState new_state, Rectangle const& new_placement) -> Rectangle override;

    /// Handles Alt-Tab, Alt-Grave and Alt-F4
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    /// Handles touch to focus
    bool handle_touch_event(MirTouchEvent const* event) override;

    /// Handles pre-existing move & resize gestures, plus click to focus
    bool handle_pointer_event(MirPointerEvent const* event) override;

    /// Currently unimplemented
    void handle_request_drag_and_drop(WindowInfo& window_info) override;

    /// Initiates a move gesture (only implemented for pointers)
    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;

    /// Initiates a resize gesture (only implemented for pointers)
    void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

    /// Honours the requested movement
    auto confirm_inherited_move(WindowInfo const& window_info, Displacement movement) -> Rectangle override;

    /// Raises newly focused window
    void advise_focus_gained(WindowInfo const& window_info) override;

protected:
    WindowManagerTools tools;
    struct Impl* const self;
};
}

#endif //MIRAL_MINIMAL_WINDOW_MANAGER_H
