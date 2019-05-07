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

#include <miral/minimal_window_manager.h>

struct miral::Impl {};


miral::MinimalWindowManager::MinimalWindowManager(WindowManagerTools const& tools):
    tools{tools},
    self{new Impl}
{
}

miral::MinimalWindowManager::~MinimalWindowManager()
{
    delete self;
}

auto miral::MinimalWindowManager::place_new_window(
    ApplicationInfo const& /*app_info*/, WindowSpecification const& requested_specification)
    -> WindowSpecification
{
    return requested_specification;
}

void miral::MinimalWindowManager::handle_window_ready(WindowInfo& window_info)
{
    if (window_info.can_be_active())
    {
        tools.select_active_window(window_info.window());
    }
}

void miral::MinimalWindowManager::handle_modify_window(
    WindowInfo& window_info, miral::WindowSpecification const& modifications)
{
    tools.modify_window(window_info, modifications);
}

void miral::MinimalWindowManager::handle_raise_window(WindowInfo& window_info)
{
    tools.select_active_window(window_info.window());
}

auto miral::MinimalWindowManager::confirm_placement_on_display(
    WindowInfo const& /*window_info*/, MirWindowState /*new_state*/, Rectangle const& new_placement)
    -> Rectangle
{
    return new_placement;
}

bool miral::MinimalWindowManager::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool miral::MinimalWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
    return false;
}

bool miral::MinimalWindowManager::handle_pointer_event(MirPointerEvent const* /*event*/)
{
    return false;
}

void miral::MinimalWindowManager::handle_request_drag_and_drop(WindowInfo& /*window_info*/)
{
    // TODO
}

void miral::MinimalWindowManager::handle_request_move(WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/)
{
    // TODO
}

void miral::MinimalWindowManager::handle_request_resize(
    WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/)
{
    // TODO
}

auto miral::MinimalWindowManager::confirm_inherited_move(WindowInfo const& window_info, Displacement movement)
-> Rectangle
{
    return {window_info.window().top_left()+movement, window_info.window().size()};
}
