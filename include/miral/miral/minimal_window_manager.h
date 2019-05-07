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

    auto place_new_window(
        ApplicationInfo const& app_info,
        WindowSpecification const& requested_specification) -> WindowSpecification override;

    void handle_window_ready(WindowInfo& window_info) override;

    void handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications) override;

    void handle_raise_window(WindowInfo& window_info) override;

    Rectangle confirm_placement_on_display(
        WindowInfo const& window_info, MirWindowState new_state, Rectangle const& new_placement) override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    bool handle_touch_event(MirTouchEvent const* event) override;

    bool handle_pointer_event(MirPointerEvent const* event) override;

    void handle_request_drag_and_drop(WindowInfo& window_info) override;

    void handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event) override;

    void handle_request_resize(WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

    Rectangle confirm_inherited_move(WindowInfo const& window_info, Displacement movement) override;

protected:
    WindowManagerTools tools;
    struct Impl* const self;
};
}

#endif //MIRAL_MINIMAL_WINDOW_MANAGER_H
