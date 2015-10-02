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

#include "server_example_tiling_window_manager.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/shell/surface_specification.h"
#include "mir/geometry/displacement.h"

#include <linux/input.h>
#include <csignal>

namespace me = mir::examples;
namespace ms = mir::scene;
namespace mf = mir::frontend;
using namespace mir::geometry;

///\example server_example_tiling_window_manager.cpp
/// Demonstrate implementing a simple tiling algorithm

me::TilingSurfaceInfo::TilingSurfaceInfo(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    scene::SurfaceCreationParameters const& /*params*/) :
    session{session},
    state{mir_surface_state_restored},
    restore_rect{surface->top_left(), surface->size()}
{
}

me::TilingWindowManagerPolicy::TilingWindowManagerPolicy(Tools* const tools) :
    tools{tools}
{
}

void me::TilingWindowManagerPolicy::click(Point cursor)
{
    auto const session = session_under(cursor);
    auto const surface = tools->surface_at(cursor);
    tools->set_focus_to(session, surface);
    if (auto const surface = tools->focused_surface())
        tools->raise({surface});
    old_cursor = cursor;
}

void me::TilingWindowManagerPolicy::handle_session_info_updated(TilingSessionInfoMap& session_info, Rectangles const& displays)
{
    update_tiles(session_info, displays);
}

void me::TilingWindowManagerPolicy::handle_displays_updated(TilingSessionInfoMap& session_info, Rectangles const& displays)
{
    update_tiles(session_info, displays);
}

void me::TilingWindowManagerPolicy::resize(Point cursor)
{
    if (auto const session = session_under(cursor))
    {
        if (session == session_under(old_cursor))
        {
            auto const& info = tools->info_for(session);

            if (resize(tools->focused_surface(), cursor, old_cursor, info.tile))
            {
                // Still dragging the same surface
            }
            else
            {
                auto const new_surface = tools->surface_at(old_cursor);

                if (new_surface && tools->info_for(new_surface).session.lock() == session)
                {
                    if (resize(new_surface, cursor, old_cursor, info.tile))
                    {
                        tools->set_focus_to(session, new_surface);
                        if (auto const surface = tools->focused_surface())
                            tools->raise({surface});
                    }
                }
            }
        }
    }

    old_cursor = cursor;
}

auto me::TilingWindowManagerPolicy::handle_place_new_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& request_parameters)
-> ms::SurfaceCreationParameters
{
    auto parameters = request_parameters;

    Rectangle const& tile = tools->info_for(session).tile;
    parameters.top_left = parameters.top_left + (tile.top_left - Point{0, 0});

    clip_to_tile(parameters, tile);
    return parameters;
}

void me::TilingWindowManagerPolicy::generate_decorations_for(
    std::shared_ptr<ms::Session> const&,
    std::shared_ptr<ms::Surface> const&,
    TilingSurfaceInfoMap&,
    std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const&, ms::SurfaceCreationParameters const&)> const&)
{
}

void me::TilingWindowManagerPolicy::handle_new_surface(std::shared_ptr<ms::Session> const& session, std::shared_ptr<ms::Surface> const& surface)
{
    tools->info_for(session).surfaces.push_back(surface);
}

void me::TilingWindowManagerPolicy::handle_modify_surface(
    std::shared_ptr<scene::Session> const& /*session*/,
    std::shared_ptr<scene::Surface> const& surface,
    shell::SurfaceSpecification const& modifications)
{
    if (modifications.name.is_set())
        surface->rename(modifications.name.value());
}

void me::TilingWindowManagerPolicy::handle_delete_surface(std::shared_ptr<ms::Session> const& session, std::weak_ptr<ms::Surface> const& surface)
{
    auto& surfaces = tools->info_for(session).surfaces;

    for (auto i = begin(surfaces); i != end(surfaces); ++i)
    {
        if (surface.lock() == i->lock())
        {
            surfaces.erase(i);
            break;
        }
    }

    if (surfaces.empty() && session == tools->focused_session())
    {
        tools->focus_next_session();
        if (auto const surface = tools->focused_surface())
            tools->raise({surface});
    }
}

int me::TilingWindowManagerPolicy::handle_set_state(std::shared_ptr<ms::Surface> const& surface, MirSurfaceState value)
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

    auto const& tile = tools->info_for(info.session).tile;

    switch (value)
    {
    case mir_surface_state_restored:
        surface->move_to(info.restore_rect.top_left);
        surface->resize(info.restore_rect.size);
        break;

    case mir_surface_state_maximized:
        surface->move_to(tile.top_left);
        surface->resize(tile.size);
        break;

    case mir_surface_state_horizmaximized:
        surface->move_to({tile.top_left.x, info.restore_rect.top_left.y});
        surface->resize({tile.size.width, info.restore_rect.size.height});
        break;

    case mir_surface_state_vertmaximized:
        surface->move_to({info.restore_rect.top_left.x, tile.top_left.y});
        surface->resize({info.restore_rect.size.width, tile.size.height});
        break;

    default:
        break;
    }

    return info.state = value;
}

void me::TilingWindowManagerPolicy::drag(Point cursor)
{
    if (auto const session = session_under(cursor))
    {
        if (session == session_under(old_cursor))
        {
            auto const& info = tools->info_for(session);
            if (drag(tools->focused_surface(), cursor, old_cursor, info.tile))
            {
                // Still dragging the same surface
            }
            else
            {
                auto const new_surface = tools->surface_at(old_cursor);

                if (new_surface && tools->info_for(new_surface).session.lock() == session)
                {
                    if (resize(new_surface, cursor, old_cursor, info.tile))
                    {
                        tools->set_focus_to(session, new_surface);
                        if (auto const surface = tools->focused_surface())
                            tools->raise({surface});
                    }
                }
            }
        }
    }

    old_cursor = cursor;
}

bool me::TilingWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

    if (action == mir_keyboard_action_down && scan_code == KEY_F11)
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
    else if (action == mir_keyboard_action_down && scan_code == KEY_F4)
    {
        if (auto const session = tools->focused_session())
        {
            switch (modifiers & modifier_mask)
            {
            case mir_input_event_modifier_alt:
                kill(session->process_id(), SIGTERM);
                return true;

            case mir_input_event_modifier_ctrl:
                if (auto const surf = session->default_surface())
                {
                    surf->request_client_surface_close();
                    return true;
                }

            default:
                break;
            }
        }
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_TAB)
    {
        tools->focus_next_session();
        if (auto const surface = tools->focused_surface())
            tools->raise({surface});

        return true;
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_GRAVE)
    {
        if (auto const prev = tools->focused_surface())
        {
            if (auto const app = tools->focused_session())
                if (auto const surface = app->surface_after(prev))
                {
                    tools->set_focus_to(app, surface);
                    tools->raise({surface});
                }
        }

        return true;
    }

    return false;
}

bool me::TilingWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event)
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

    bool is_drag = true;
    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_event_action(event, i))
        {
        case mir_touch_action_up:
            return false;

        case mir_touch_action_down:
            is_drag = false;

        case mir_touch_action_change:
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

bool me::TilingWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const modifiers = mir_pointer_event_modifiers(event) & modifier_mask;
    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    if (action == mir_pointer_action_button_down)
    {
        click(cursor);
        return false;
    }
    else if (action == mir_pointer_action_motion &&
             modifiers == mir_input_event_modifier_alt)
    {
        if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
        {
            drag(cursor);
            return true;
        }

        if (mir_pointer_event_button_state(event, mir_pointer_button_tertiary))
        {
            resize(cursor);
            return true;
        }
    }

    return false;
}

void me::TilingWindowManagerPolicy::toggle(MirSurfaceState state)
{
    if (auto const session = tools->focused_session())
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

std::shared_ptr<ms::Session> me::TilingWindowManagerPolicy::session_under(Point position)
{
    return tools->find_session([&](TilingSessionInfo const& info) { return info.tile.contains(position);});
}

void me::TilingWindowManagerPolicy::update_tiles(
    TilingSessionInfoMap& session_info,
    Rectangles const& displays)
{
    if (session_info.size() < 1 || displays.size() < 1) return;

    auto const sessions = session_info.size();

    auto const bounding_rect = displays.bounding_rectangle();

    auto const total_width  = bounding_rect.size.width.as_int();
    auto const total_height = bounding_rect.size.height.as_int();

    auto index = 0;

    for (auto& info : session_info)
    {
        auto const x = (total_width*index)/sessions;
        ++index;
        auto const dx = (total_width*index)/sessions - x;

        auto const old_tile = info.second.tile;
        Rectangle const new_tile{{x, 0}, {dx, total_height}};

        update_surfaces(info.first, old_tile, new_tile);

        info.second.tile = new_tile;
    }
}

void me::TilingWindowManagerPolicy::update_surfaces(std::weak_ptr<ms::Session> const& session, Rectangle const& old_tile, Rectangle const& new_tile)
{
    auto displacement = new_tile.top_left - old_tile.top_left;
    auto& info = tools->info_for(session);

    for (auto const& ps : info.surfaces)
    {
        if (auto const surface = ps.lock())
        {
            auto const old_pos = surface->top_left();
            surface->move_to(old_pos + displacement);

            fit_to_new_tile(*surface, old_tile, new_tile);
        }
    }
}

void me::TilingWindowManagerPolicy::clip_to_tile(ms::SurfaceCreationParameters& parameters, Rectangle const& tile)
{
    auto const displacement = parameters.top_left - tile.top_left;

    auto width = std::min(tile.size.width.as_int()-displacement.dx.as_int(), parameters.size.width.as_int());
    auto height = std::min(tile.size.height.as_int()-displacement.dy.as_int(), parameters.size.height.as_int());

    parameters.size = Size{width, height};
}

void me::TilingWindowManagerPolicy::fit_to_new_tile(ms::Surface& surface, Rectangle const& old_tile, Rectangle const& new_tile)
{
    auto const displacement = surface.top_left() - new_tile.top_left;

    // For now just scale if was filling width/height of tile
    auto const old_size = surface.size();
    auto const scaled_width = old_size.width == old_tile.size.width ? new_tile.size.width : old_size.width;
    auto const scaled_height = old_size.height == old_tile.size.height ? new_tile.size.height : old_size.height;

    auto width = std::min(new_tile.size.width.as_int()-displacement.dx.as_int(), scaled_width.as_int());
    auto height = std::min(new_tile.size.height.as_int()-displacement.dy.as_int(), scaled_height.as_int());

    surface.resize({width, height});
}

bool me::TilingWindowManagerPolicy::drag(std::shared_ptr<ms::Surface> surface, Point to, Point from, Rectangle bounds)
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

bool me::TilingWindowManagerPolicy::resize(std::shared_ptr<ms::Surface> surface, Point cursor, Point old_cursor, Rectangle bounds)
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
