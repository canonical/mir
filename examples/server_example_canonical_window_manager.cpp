/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_canonical_window_manager.h"

#include "mir/scene/surface.h"
#include "mir/geometry/displacement.h"

#include <linux/input.h>

namespace me = mir::examples;
namespace ms = mir::scene;
using namespace mir::geometry;

///\example server_example_canonical_window_manager.cpp
/// Demonstrate implementing a standard tiling algorithm

namespace
{
void clip_to_area(ms::SurfaceCreationParameters& parameters, Rectangle const& area)
{
    auto const displacement = parameters.top_left - area.top_left;

    auto width = std::min(area.size.width.as_int()-displacement.dx.as_int(), parameters.size.width.as_int());
    auto height = std::min(area.size.height.as_int()-displacement.dy.as_int(), parameters.size.height.as_int());

    parameters.size = Size{width, height};
}

bool resize(std::shared_ptr<ms::Surface> const& surface, Point cursor, Point old_cursor, Rectangle bounds)
{
    if (surface && surface->input_area_contains(old_cursor))
    {
        auto const top_left = surface->top_left();

        auto const old_displacement = old_cursor - top_left;
        auto const new_displacement = cursor - top_left;

        auto const scale_x = new_displacement.dx.as_float()/std::max(1.0f, old_displacement.dx.as_float());
        auto const scale_y = new_displacement.dy.as_float()/std::max(1.0f, old_displacement.dy.as_float());

        if (scale_x <= 0.0f || scale_y <= 0.0f) return false;

        auto const old_size = surface->size();
        Size new_size{scale_x*old_size.width, scale_y*old_size.height};

        auto const size_limits = as_size(bounds.bottom_right() - top_left);

        if (new_size.width > size_limits.width)
            new_size.width = size_limits.width;

        if (new_size.height > size_limits.height)
            new_size.height = size_limits.height;

        surface->resize(new_size);

        return true;
    }

    return false;
}

bool drag(std::shared_ptr<ms::Surface> surface, Point to, Point from, Rectangle bounds)
{
    if (surface && surface->input_area_contains(from))
    {
        auto const top_left = surface->top_left();
        auto const surface_size = surface->size();
        auto const bottom_right = top_left + as_displacement(surface_size);

        auto movement = to - from;

        if (movement.dx < DeltaX{0})
            movement.dx = std::max(movement.dx, (bounds.top_left - top_left).dx);

        if (movement.dy < DeltaY{0})
            movement.dy = std::max(movement.dy, (bounds.top_left - top_left).dy);

        if (movement.dx > DeltaX{0})
            movement.dx = std::min(movement.dx, (bounds.bottom_right() - bottom_right).dx);

        if (movement.dy > DeltaY{0})
            movement.dy = std::min(movement.dy, (bounds.bottom_right() - bottom_right).dy);

        auto new_pos = surface->top_left() + movement;

        surface->move_to(new_pos);
        return true;
    }

    return false;
}
}

me::CanonicalSurfaceInfo::CanonicalSurfaceInfo(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface) :
    state{mir_surface_state_restored},
    restore_rect{surface->top_left(), surface->size()},
    session{session}
{
}

me::CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy(Tools* const tools,
    std::shared_ptr<graphics::Display> const& display,
    std::shared_ptr<compositor::Compositor> const& compositor) :
    tools{tools}, display{display}, compositor{compositor}
{
}

void me::CanonicalWindowManagerPolicy::click(Point cursor)
{
    // TODO need to find the clicked surface & session and set the focus & old_surface
    old_cursor = cursor;
}

void me::CanonicalWindowManagerPolicy::handle_session_info_updated(CanonicalSessionInfoMap& /*session_info*/, Rectangles const& /*displays*/)
{
}

void me::CanonicalWindowManagerPolicy::handle_displays_updated(CanonicalSessionInfoMap& /*session_info*/, Rectangles const& displays)
{
    display_area = displays.bounding_rectangle();
}

void me::CanonicalWindowManagerPolicy::resize(Point cursor)
{
    if (::resize(old_surface.lock(), cursor, old_cursor, display_area))
    {
        return;
    }

    if (auto const session = tools->focussed_application().lock())
    {
        if (::resize(session->default_surface(), cursor, old_cursor, display_area))
        {
            old_surface = session->default_surface();
        }
    }

    old_cursor = cursor;
}

auto me::CanonicalWindowManagerPolicy::handle_place_new_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    ms::SurfaceCreationParameters const& request_parameters)
-> ms::SurfaceCreationParameters
{
    auto parameters = request_parameters;

    // TODO position window

    clip_to_area(parameters, display_area);
    return parameters;
}

void me::CanonicalWindowManagerPolicy::handle_new_surface(std::shared_ptr<ms::Session> const& /*session*/, std::shared_ptr<ms::Surface> const& /*surface*/)
{
    // TODO
}

void me::CanonicalWindowManagerPolicy::handle_delete_surface(std::shared_ptr<ms::Session> const& /*session*/, std::weak_ptr<ms::Surface> const& /*surface*/)
{
    // TODO
}

int me::CanonicalWindowManagerPolicy::handle_set_state(std::shared_ptr<ms::Surface> const& surface, MirSurfaceState value)
{
    auto& info = tools->info_for(surface);

    switch (value)
    {
    case mir_surface_state_restored:
    case mir_surface_state_maximized:
    case mir_surface_state_vertmaximized:
    case mir_surface_state_horizmaximized:
        break;

    default:
        return info.state;
    }

    if (info.state == mir_surface_state_restored)
    {
        info.restore_rect = {surface->top_left(), surface->size()};
    }

    if (info.state == value)
    {
        return info.state;
    }

    switch (value)
    {
    case mir_surface_state_restored:
        surface->move_to(info.restore_rect.top_left);
        surface->resize(info.restore_rect.size);
        break;

    case mir_surface_state_maximized:
        surface->move_to(display_area.top_left);
        surface->resize(display_area.size);
        break;

    case mir_surface_state_horizmaximized:
        surface->move_to({display_area.top_left.x, info.restore_rect.top_left.y});
        surface->resize({display_area.size.width, info.restore_rect.size.height});
        break;

    case mir_surface_state_vertmaximized:
        surface->move_to({info.restore_rect.top_left.x, display_area.top_left.y});
        surface->resize({info.restore_rect.size.width, display_area.size.height});
        break;

    default:
        break;
    }

    return info.state = value;
}

void me::CanonicalWindowManagerPolicy::drag(Point cursor)
{
    if (::drag(old_surface.lock(), cursor, old_cursor, display_area))
    {
        return;
    }

    if (auto const session = tools->focussed_application().lock())
    {
        if (::drag(session->default_surface(), cursor, old_cursor, display_area))
        {
            old_surface = session->default_surface();
        }
    }

    old_cursor = cursor;
}

bool me::CanonicalWindowManagerPolicy::handle_key_event(MirKeyInputEvent const* event)
{
    auto const action = mir_key_input_event_get_action(event);
    auto const scan_code = mir_key_input_event_get_scan_code(event);
    auto const modifiers = mir_key_input_event_get_modifiers(event) & modifier_mask;

    if (action == mir_key_input_event_action_down && scan_code == KEY_F11)
    {
        switch (modifiers & modifier_mask)
        {
        case mir_input_event_modifier_alt:
            toggle(mir_surface_state_maximized);
            return true;

        case mir_input_event_modifier_shift:
            toggle(mir_surface_state_vertmaximized);
            return true;

        case mir_input_event_modifier_ctrl:
            toggle(mir_surface_state_horizmaximized);
            return true;

        default:
            break;
        }
    }

    return false;
}

bool me::CanonicalWindowManagerPolicy::handle_touch_event(MirTouchInputEvent const* event)
{
    auto const count = mir_touch_input_event_get_touch_count(event);

    long total_x = 0;
    long total_y = 0;

    for (auto i = 0U; i != count; ++i)
    {
        total_x += mir_touch_input_event_get_touch_axis_value(event, i, mir_touch_input_axis_x);
        total_y += mir_touch_input_event_get_touch_axis_value(event, i, mir_touch_input_axis_y);
    }

    Point const cursor{total_x/count, total_y/count};

    bool is_drag = true;
    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_input_event_get_touch_action(event, i))
        {
        case mir_touch_input_event_action_up:
            return false;

        case mir_touch_input_event_action_down:
            is_drag = false;

        case mir_touch_input_event_action_change:
            continue;
        }
    }

    if (is_drag && count == 3)
    {
        drag(cursor);
        return true;
    }
    else
    {
        click(cursor);
        return false;
    }
}

bool me::CanonicalWindowManagerPolicy::handle_pointer_event(MirPointerInputEvent const* event)
{
    auto const action = mir_pointer_input_event_get_action(event);
    auto const modifiers = mir_pointer_input_event_get_modifiers(event) & modifier_mask;
    Point const cursor{
        mir_pointer_input_event_get_axis_value(event, mir_pointer_input_axis_x),
        mir_pointer_input_event_get_axis_value(event, mir_pointer_input_axis_y)};

    if (action == mir_pointer_input_event_action_button_down)
    {
        click(cursor);
        return false;
    }
    else if (action == mir_pointer_input_event_action_motion &&
             modifiers == mir_input_event_modifier_alt)
    {
        if (mir_pointer_input_event_get_button_state(event, mir_pointer_input_button_primary))
        {
            drag(cursor);
            return true;
        }

        if (mir_pointer_input_event_get_button_state(event, mir_pointer_input_button_tertiary))
        {
            resize(cursor);
            return true;
        }
    }

    return false;
}

void me::CanonicalWindowManagerPolicy::toggle(MirSurfaceState state)
{
    if (auto const session = tools->focussed_application().lock())
    {
        if (auto const surface = session->default_surface())
        {
            if (surface->state() == state)
                state = mir_surface_state_restored;

            auto const value = handle_set_state(surface, MirSurfaceState(state));
            surface->configure(mir_surface_attrib_state, value);
        }
    }
}
