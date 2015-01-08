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
#include "mir/scene/session_listener.h"
#include "mir/shell/focus_controller.h"

#include <map>
#include <vector>
#include <mutex>

namespace mc = mir::compositor;
namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
using namespace mir::geometry;

///\example server_example_window_management.cpp
/// Demonstrate simple window management strategies

namespace
{
// Very simple - make every surface fullscreen
class FullscreenWindowManager : public me::WindowManager, me::FullscreenPlacementStrategy
{
public:
    using me::FullscreenPlacementStrategy::FullscreenPlacementStrategy;

private:
    void add_surface(std::shared_ptr<ms::Surface> const&, ms::Session*) override {}

    void remove_surface(std::weak_ptr<ms::Surface> const&, ms::Session*) override {}

    void add_session(std::shared_ptr<ms::Session> const&) override {}

    void remove_session(std::shared_ptr<ms::Session> const&) override {}

    void add_display(Rectangle const&) override {}

    void remove_display(Rectangle const&) override {}

    void click(Point) override {}

    void drag(Point) override {}
};

// simple tiling algorithm
class TilingWindowManager : public me::WindowManager
{
public:
    // TODO we can't take the msh::FocusController directly as we create a WindowManager
    // TODO in the_session_listener() and that is called when creating the focus controller
    using FocusControllerFactory = std::function<std::shared_ptr<msh::FocusController>()>;

    explicit TilingWindowManager(FocusControllerFactory const& focus_controller) :
        focus_controller{focus_controller}
    {
    }

    auto place(ms::Session const& session, ms::SurfaceCreationParameters const& request_parameters)
    -> ms::SurfaceCreationParameters override
    {
        auto parameters = request_parameters;

        std::lock_guard<decltype(mutex)> lock(mutex);
        auto const ptile = session_info.find(&session);
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
        ms::Session* session) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info[session].surfaces.push_back(surface);
    }

    void remove_surface(
        std::weak_ptr<ms::Surface> const& surface,
        ms::Session* session) override
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
    }

    void add_session(std::shared_ptr<ms::Session> const& session) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info[session.get()] = session;
        update_tiles();
    }

    void remove_session(std::shared_ptr<ms::Session> const& session) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        session_info.erase(session.get());
        update_tiles();
    }

    void add_display(Rectangle const& area) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        displays.push_back(area);
        update_tiles();
    }

    void remove_display(Rectangle const& area) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto const i = std::find(begin(displays), end(displays), area);
        if (i != end(displays)) displays.erase(i);
        update_tiles();
    }

    void click(Point cursor) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (auto const session = session_under(cursor))
            focus_controller()->set_focus_to(session);

        old_cursor = cursor;
    }

    void drag(Point cursor) override
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (auto const session = session_under(cursor))
        {
            if (session == session_under(old_cursor))
            {
                auto& info = session_info[session.get()];

                if (drag(old_surface.lock(), cursor, old_cursor, info.tile))
                {
                    // DONE
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

private:
    void update_tiles()
    {
        if (session_info.size() < 1 || displays.size() < 1) return;

        auto const sessions = session_info.size();
        Rectangles view;

        for (auto const& display : displays)
            view.add(display);

        auto const bounding_rect = view.bounding_rectangle();

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

    void update_surfaces(ms::Session const* session, Rectangle const& old_tile, Rectangle const& new_tile)
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
            auto const movement = to - from;
            auto const new_pos = surface->top_left() + movement;
            auto const surface_size = surface->size();
            auto const bottom_right = new_pos + Displacement{surface_size.width.as_int(), surface_size.height.as_int()};

            if ((movement.dx < DeltaX{0} || movement.dy < DeltaY{0}) && !bounds.contains(new_pos)) return true;
            if ((movement.dx > DeltaX{0} || movement.dy > DeltaY{0}) && !bounds.contains(bottom_right)) return true;

            surface->move_to(new_pos);
            return true;
        }

        return false;
    }

    std::shared_ptr<ms::Session> session_under(Point position)
    {
        for(auto& info : session_info)
        {
            if (info.second.tile.contains(position))
            {
                return info.second.session.lock();
            }
        }

        return std::shared_ptr<ms::Session>{};
    }

    FocusControllerFactory const focus_controller;

    std::mutex mutex;
    std::vector<Rectangle> displays;

    struct SessionInfo
    {
        SessionInfo() = default;
        SessionInfo& operator=(std::weak_ptr<ms::Session> const& session)
        {
            this->session = session;
            surfaces.clear();
            return *this;
        }
        std::weak_ptr<ms::Session> session;
        Rectangle tile;
        std::vector<std::weak_ptr<ms::Surface>> surfaces;
    };

    std::map<ms::Session const*, SessionInfo> session_info;
    Point old_cursor{};
    std::weak_ptr<ms::Surface> old_surface;
};
}

class me::EventTracker : public mi::EventFilter
{
public:
    explicit EventTracker(std::shared_ptr<me::WindowManager> const& window_manager) :
        window_manager{window_manager} {}

    bool handle(MirEvent const& event) override
    {
        switch (event.type)
        {
        case mir_event_type_key:
            return handle_key_event(event.key);

        case mir_event_type_motion:
            return handle_motion_event(event.motion);

        default:
            return false;
        }
    }

private:
    bool handle_key_event(MirKeyEvent const& /*event*/)
    {
        return false;
    }

    bool handle_motion_event(MirMotionEvent const& event)
    {
        if (event.action == mir_motion_action_down ||
            event.action == mir_motion_action_pointer_down)
        {
            if (auto const wm = window_manager.lock())
            {
                wm->click(average_pointer(event.pointer_count, event.pointer_coordinates));
                return true;
            }
        }
        else if (event.action == mir_motion_action_move &&
                 event.modifiers & mir_key_modifier_alt)
        {
            if (auto const wm = window_manager.lock())
            {
                wm->drag(average_pointer(event.pointer_count, event.pointer_coordinates));
                return true;
            }
        }

        return false;
    }

    static Point average_pointer(size_t pointer_count, MirMotionPointer const* pointer_coordinates)
    {
        long total_x = 0;
        long total_y = 0;

        for (auto p = pointer_coordinates; p != pointer_coordinates + pointer_count; ++p)
        {
            total_x += p->x;
            total_y += p->y;
        }

        return Point{total_x/pointer_count, total_y/pointer_count};
    }

    std::weak_ptr<me::WindowManager> const window_manager;
};

char const* const me::wm_option = "window-manager";
char const* const me::wm_description = "window management strategy [{tiling|fullscreen}]";

namespace
{
char const* const wm_tiling = "tiling";
char const* const wm_fullscreen = "fullscreen";
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
            auto focus_controller_factory = [this] { return server.the_focus_controller(); };
            tmp = std::make_shared<TilingWindowManager>(focus_controller_factory);
        }
        else if (selection == wm_fullscreen)
            tmp = std::make_shared<FullscreenWindowManager>(server.the_shell_display_layout());
        else
            throw mir::AbnormalExit("Unknown window manager: " + selection);

        et = std::make_shared<EventTracker>(tmp);
        server.the_composite_event_filter()->prepend(et);
        wm = tmp;
    }

    return tmp;
}
