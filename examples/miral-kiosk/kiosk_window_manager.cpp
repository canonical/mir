/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "kiosk_window_manager.h"

#include <miral/application_info.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

#include <linux/input.h>

namespace ms = mir::scene;
using namespace miral;

KioskWindowManagerPolicy::KioskWindowManagerPolicy(WindowManagerTools const& tools, SwSplash const& splash) :
    CanonicalWindowManagerPolicy{tools},
    splash{splash}
{
}

bool KioskWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

    if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_TAB)
    {
        tools.focus_next_application();

        return true;
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_GRAVE)
    {
        tools.focus_next_within_application();

        return true;
    }
    else if (action == mir_keyboard_action_down &&
             modifiers == (mir_input_event_modifier_alt | mir_input_event_modifier_shift) &&
             scan_code == KEY_GRAVE)
    {
        tools.focus_prev_within_application();

        return true;
    }
    else if (action == mir_keyboard_action_down && scan_code == KEY_F4)
    {
        switch (modifiers & modifier_mask)
        {
        case mir_input_event_modifier_alt:
            tools.ask_client_to_close(tools.active_window());;
            return true;

        default:
            break;
        }
    }

    return false;
}

bool KioskWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event)
{
    auto const count = mir_touch_event_point_count(event);

    long total_x = 0;
    long total_y = 0;

    for (auto i = 0U; i != count; ++i)
    {
        total_x += mir_touch_event_axis_value(event, i, mir_touch_axis_x);
        total_y += mir_touch_event_axis_value(event, i, mir_touch_axis_y);
    }

    Point const cursor{total_x/count, total_y/count};

    tools.select_active_window(tools.window_at(cursor));

    return false;
}

bool KioskWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);

    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    if (action == mir_pointer_action_button_down)
    {
        tools.select_active_window(tools.window_at(cursor));
    }

    return false;
}

void KioskWindowManagerPolicy::advise_focus_gained(WindowInfo const& info)
{
    CanonicalWindowManagerPolicy::advise_focus_gained(info);

    if (auto session = splash.session().lock())
    {
        auto const& app_info = tools.info_for(session);

        for (auto const& s : app_info.windows())
            tools.raise_tree(s);
    }
}

void KioskWindowManagerPolicy::advise_new_window(WindowInfo const& window_info)
{
    // We do this here, not in place_new_window() so that clients get a resize event.
    // This shouldn't be necessary, but works better with the gtk-mir backend.
    if (window_info.type() == mir_window_type_normal &&
        !window_info.parent() &&
        window_info.state() == mir_window_state_restored)
    {
        WindowSpecification specification;

        specification.state() = mir_window_state_maximized;

        tools.place_and_size_for_state(specification, window_info);
        tools.modify_window(window_info.window(), specification);
    }

    CanonicalWindowManagerPolicy::advise_new_window(window_info);
}
