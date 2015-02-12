/*
 * Copyright © 2014 Canonical Ltd.
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

#include "server_example_window_management.h"
#include "server_example_fullscreen_placement_strategy.h"

#include "mir/abnormal_exit.h"
#include "mir/server.h"
#include "mir/geometry/rectangles.h"
#include "mir/geometry/displacement.h"
#include "mir/input/composite_event_filter.h"
#include "mir/options/option.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/abstract_shell.h"

#include "mir_toolkit/events/input/input_event.h"

#include <linux/input.h>

#include <map>
#include <vector>
#include <mutex>

namespace me = mir::examples;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
using namespace mir::geometry;

///\example server_example_window_management.cpp
/// Demonstrate simple window management strategies

char const* const me::wm_option = "window-manager";
char const* const me::wm_description = "window management strategy [{tiling|fullscreen}]";

namespace
{
char const* const wm_tiling = "tiling";
char const* const wm_fullscreen = "fullscreen";

// Very simple - make every surface fullscreen
class FullscreenWindowManager : public me::WindowManager,
    msh::AbstractShell,
    me::FullscreenPlacementStrategy
{
public:
    FullscreenWindowManager(
        std::shared_ptr<msh::InputTargeter> const& input_targeter,
        std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<ms::PromptSessionManager> const& prompt_session_manager,
        std::shared_ptr<msh::DisplayLayout> const& display_layout) :
        AbstractShell(input_targeter, surface_coordinator, session_coordinator, prompt_session_manager),
        FullscreenPlacementStrategy(display_layout)
    {
    }

private:
    mf::SurfaceId create_surface(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params) override
    {
        ms::SurfaceCreationParameters const placed_params = place(*session, params);
        return msh::AbstractShell::create_surface(session, placed_params);
    }

    void add_display(Rectangle const&) override {}

    void remove_display(Rectangle const&) override {}

    bool handle(MirEvent const& /*event*/) override { return false; }
};

// simple tiling algorithm:
//  o Switch apps: tap or click on the corresponding tile
//  o Move window: Alt-leftmousebutton drag
//  o Resize window: Alt-middle_button drag
//  o Maximize/restore current window (to tile size): Alt-F11
//  o Maximize/restore current window (to tile height): Shift-F11
//  o Maximize/restore current window (to tile width): Ctrl-F11
//  o client requests to maximize, vertically maximize & restore
class TilingWindowManager : public me::WindowManager,
    msh::AbstractShell
{
public:
    using msh::AbstractShell::AbstractShell;

private:
    void add_display(Rectangle const& area) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        displays.add(area);
        update_tiles();
    }

    void remove_display(Rectangle const& area) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        displays.remove(area);
        update_tiles();
    }

    bool handle(MirEvent const& event) override
    {
        if (mir_event_get_type(&event) != mir_event_type_input)
            return false;

        auto const input_event = mir_event_get_input_event(&event);

        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            return handle_key_event(mir_input_event_get_key_input_event(input_event));

        case mir_input_event_type_touch:
            return handle_touch_event(mir_input_event_get_touch_input_event(input_event));

        case mir_input_event_type_pointer:
            return handle_pointer_event(mir_input_event_get_pointer_input_event(input_event));
        }

        return false;
    }

    static const int modifier_mask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

    bool handle_key_event(MirKeyInputEvent const* event)
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

    bool handle_touch_event(MirTouchInputEvent const* event)
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

    bool handle_pointer_event(MirPointerInputEvent const* event)
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

    void click(Point cursor)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (auto const session = session_under(cursor))
            set_focus_to(session);

        old_cursor = cursor;
    }

    void drag(Point cursor)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (auto const session = session_under(cursor))
        {
            if (session == session_under(old_cursor))
            {
                auto const& info = session_info[session];

                if (drag(old_surface.lock(), cursor, old_cursor, info.tile))
                {
                    // Still dragging the same old_surface
                }
                else if (drag(session->default_surface(), cursor, old_cursor, info.tile))
                {
                    old_surface = session->default_surface();
                }
                else
                {
                    for (auto const& ps : info.surfaces)
                    {
                        auto const new_surface = ps.lock();

                        if (drag(new_surface, cursor, old_cursor, info.tile))
                        {
                            old_surface = new_surface;
                            break;
                        }
                    }
                }
            }
        }

        old_cursor = cursor;
    }

    void resize(Point cursor)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (auto const session = session_under(cursor))
        {
            if (session == session_under(old_cursor))
            {
                auto const& info = session_info[session];

                if (resize(old_surface.lock(), cursor, old_cursor, info.tile))
                {
                    // Still dragging the same old_surface
                }
                else if (resize(session->default_surface(), cursor, old_cursor, info.tile))
                {
                    old_surface = session->default_surface();
                }
                else
                {
                    for (auto const& ps : info.surfaces)
                    {
                        auto const new_surface = ps.lock();

                        if (resize(new_surface, cursor, old_cursor, info.tile))
                        {
                            old_surface = new_surface;
                            break;
                        }
                    }
                }
            }
        }

        old_cursor = cursor;
    }

    void toggle(MirSurfaceState state)
    {
        if (auto const focussed_session = focussed_application().lock())
        {
            if (auto const focussed_surface = focussed_session->default_surface())
            {
                {
                    std::lock_guard<decltype(mutex)> lock(mutex);

                    if (surface_info[focussed_surface].state == state)
                        state = mir_surface_state_restored;
                }

                set_surface_attribute(focussed_session, focussed_surface, mir_surface_attrib_state, state);
            }
        }
    }

    std::shared_ptr<ms::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<mf::EventSink> const& sink) override
    {
        auto const result = msh::AbstractShell::open_session(client_pid, name, sink);
        add_session(result);
        return result;
    }

    void close_session(std::shared_ptr<ms::Session> const& session) override
    {
        remove_session(session);
        msh::AbstractShell::close_session(session);
    }

    mf::SurfaceId create_surface(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params) override
    {
        ms::SurfaceCreationParameters const placed_params = place_new_surface(session, params);
        auto const result = msh::AbstractShell::create_surface(session, placed_params);
        add_surface(session->surface(result), session);
        return result;
    }

    void destroy_surface(std::shared_ptr<ms::Session> const& session, mf::SurfaceId surface) override
    {
        remove_surface(session->surface(surface), session);
        msh::AbstractShell::destroy_surface(session, surface);
    }

    int set_surface_attribute(
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<ms::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override
    {
        switch (attrib)
        {
        case mir_surface_attrib_state:
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            set_state(surface, MirSurfaceState(value));
            break;
        }
        default:
            break;
        }

        return msh::AbstractShell::set_surface_attribute(session, surface, attrib, value);
    }

    void add_session(std::shared_ptr<ms::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info[session] = SessionInfo();
        update_tiles();
    }

    void remove_session(std::shared_ptr<ms::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info.erase(session);
        update_tiles();
    }

    auto place_new_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& request_parameters)
    -> ms::SurfaceCreationParameters
    {
        auto parameters = request_parameters;

        std::lock_guard<decltype(mutex)> lock(mutex);
        auto const ptile = session_info.find(session);
        if (ptile != end(session_info))
        {
            Rectangle const& tile = ptile->second.tile;
            parameters.top_left = parameters.top_left + (tile.top_left - Point{0, 0});

            clip_to_tile(parameters, tile);
        }

        return parameters;
    }

    void add_surface(
        std::shared_ptr<ms::Surface> const& surface,
        std::shared_ptr<ms::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info[session].surfaces.push_back(surface);
        surface_info[surface].session = session;
        surface_info[surface].state = mir_surface_state_restored;
    }

    void remove_surface(
        std::weak_ptr<ms::Surface> const& surface,
        std::shared_ptr<ms::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto& surfaces = session_info[session].surfaces;

        for (auto i = begin(surfaces); i != end(surfaces); ++i)
        {
            if (surface.lock() == i->lock())
            {
                surfaces.erase(i);
                break;
            }
        }

        surface_info.erase(surface);
    }

    void update_tiles()
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

    void update_surfaces(std::weak_ptr<ms::Session> const& session, Rectangle const& old_tile, Rectangle const& new_tile)
    {
        auto displacement = new_tile.top_left - old_tile.top_left;
        auto& info = session_info[session];

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

    static void clip_to_tile(ms::SurfaceCreationParameters& parameters, Rectangle const& tile)
    {
        auto const displacement = parameters.top_left - tile.top_left;

        auto width = std::min(tile.size.width.as_int()-displacement.dx.as_int(), parameters.size.width.as_int());
        auto height = std::min(tile.size.height.as_int()-displacement.dy.as_int(), parameters.size.height.as_int());

        parameters.size = Size{width, height};
    }

    static void fit_to_new_tile(ms::Surface& surface, Rectangle const& old_tile, Rectangle const& new_tile)
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

    static bool drag(std::shared_ptr<ms::Surface> surface, Point to, Point from, Rectangle bounds)
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

    static bool resize(std::shared_ptr<ms::Surface> surface, Point cursor, Point old_cursor, Rectangle bounds)
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

    struct SurfaceInfo
    {
        SurfaceInfo() = default;
        std::weak_ptr<ms::Session> session;
        MirSurfaceState state;
        Rectangle restore_rect;
    };

    void set_state(std::shared_ptr<ms::Surface> const& surface, MirSurfaceState value)
    {
        switch (value)
        {
        case mir_surface_state_restored:
        case mir_surface_state_maximized:
        case mir_surface_state_vertmaximized:
        case mir_surface_state_horizmaximized:
            break;

        default:
            return;
        }

        auto& info = surface_info[surface];

        if (info.state == mir_surface_state_restored)
        {
            info.restore_rect = {surface->top_left(), surface->size()};
        }

        if (info.state == value)
        {
            return; // Nothing to do
        }

        auto const& tile = this->session_info[info.session].tile;

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

        info.state = value;
    }

    std::shared_ptr<ms::Session> session_under(Point position)
    {
        for(auto& info : session_info)
        {
            if (info.second.tile.contains(position))
            {
                return info.first.lock();
            }
        }

        return std::shared_ptr<ms::Session>{};
    }

    struct SessionInfo
    {
        Rectangle tile;
        std::vector<std::weak_ptr<ms::Surface>> surfaces;
    };

    std::mutex mutex;
    Rectangles displays;

    std::map<std::weak_ptr<ms::Session>, SessionInfo, std::owner_less<std::weak_ptr<ms::Session>>> session_info;
    std::map<std::weak_ptr<ms::Surface>, SurfaceInfo, std::owner_less<std::weak_ptr<ms::Surface>>> surface_info;

    Point old_cursor{};
    std::weak_ptr<ms::Surface> old_surface;
};
}

auto me::WindowManagmentFactory::window_manager() -> std::shared_ptr<me::WindowManager>
{
    auto tmp = wm.lock();

    if (!tmp)
    {
        auto const options = server.get_options();
        auto const selection = options->get<std::string>(wm_option);

        if (selection == wm_tiling)
        {
            tmp = std::make_shared<TilingWindowManager>(
                server.the_input_targeter(),
                server.the_surface_coordinator(),
                server.the_session_coordinator(),
                server.the_prompt_session_manager());
        }
        else if (selection == wm_fullscreen)
            tmp = std::make_shared<FullscreenWindowManager>(
                server.the_input_targeter(),
                server.the_surface_coordinator(),
                server.the_session_coordinator(),
                server.the_prompt_session_manager(),
                server.the_shell_display_layout());
        else
            throw mir::AbnormalExit("Unknown window manager: " + selection);

        server.the_composite_event_filter()->prepend(tmp);
        wm = tmp;
    }

    return tmp;
}
