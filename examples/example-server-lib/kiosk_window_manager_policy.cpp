/*
 * Copyright © Canonical Ltd.
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
 */

#include "kiosk_window_manager_policy.h"

#include <miral/application_info.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

namespace ms = mir::scene;
using namespace miral;
using namespace miral::toolkit;

KioskWindowManagerPolicyBase::KioskWindowManagerPolicyBase(WindowManagerTools const& tools, MirWindowState target_state) :
    CanonicalWindowManagerPolicy{tools},
    target_state{target_state}
{
}

bool KioskWindowManagerPolicyBase::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool KioskWindowManagerPolicyBase::handle_touch_event(MirTouchEvent const* event)
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

bool KioskWindowManagerPolicyBase::handle_pointer_event(MirPointerEvent const* event)
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

auto KioskWindowManagerPolicyBase::place_new_window(ApplicationInfo const& app_info, WindowSpecification const& request)
-> WindowSpecification
{
    WindowSpecification specification = CanonicalWindowManagerPolicy::place_new_window(app_info, request);

    if ((specification.type() == mir_window_type_normal || specification.type() == mir_window_type_freestyle) &&
        (!specification.parent().has_value() || !specification.parent().value().lock()))
    {
        specification.state() = target_state;
        specification.size() = std::nullopt; // Ignore requested size (if any) when we change state
        specification.top_left() = std::nullopt; // Ignore requested position (if any) when we change state
        tools.place_and_size_for_state(specification, WindowInfo{});

        if (!request.state().has_value() || request.state().value() != mir_window_state_restored)
            specification.state() = request.state();
    }

    return specification;
}

void KioskWindowManagerPolicyBase::handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications)
{
    WindowSpecification specification = modifications;

    if ((window_info.type() == mir_window_type_normal || window_info.type() == mir_window_type_freestyle) &&
        !window_info.parent())
    {

        if (window_info.is_visible() || !modifications.state().has_value() || modifications.state().value() != mir_window_state_restored)
        {
            specification.state() = target_state;
            specification.size() = std::nullopt; // Ignore requested size (if any) when we change state
            specification.top_left() = std::nullopt; // Ignore requested position (if any) when we change state
            tools.place_and_size_for_state(specification, window_info);

            if (!modifications.state().has_value() || modifications.state().value() != mir_window_state_restored)
                specification.state() = modifications.state();
        }
    }

    CanonicalWindowManagerPolicy::handle_modify_window(window_info, specification);
}

void KioskWindowManagerPolicyBase::handle_request_move(WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/)
{
}

void KioskWindowManagerPolicyBase::handle_request_resize(WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/)
{
}
