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

#include "server_example_window_manager.h"
#include "server_example_fullscreen_placement_strategy.h"

#include "mir/abnormal_exit.h"
#include "mir/server.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/geometry/rectangles.h"
#include "mir/geometry/displacement.h"
#include "mir/graphics/display_buffer.h"
#include "mir/options/option.h"
#include "mir/scene/observer.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/session_coordinator_wrapper.h"
#include "mir/shell/surface_coordinator_wrapper.h"

#include <algorithm>
#include <map>
#include <mutex>

namespace mc = mir::compositor;
namespace me = mir::examples;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
using namespace mir::geometry;

///\example server_example_window_manager.cpp
/// Demonstrate a simple window management strategy

namespace
{
class WindowManager : public virtual ms::PlacementStrategy
{
public:

    virtual void add_surface(
        std::shared_ptr<ms::Surface> const& surface,
        ms::Session* session) = 0;

    virtual void remove_surface(std::weak_ptr<ms::Surface> const& surface) = 0;

    virtual void add_session(std::shared_ptr<mf::Session> const& session) = 0;

    virtual void remove_session(std::shared_ptr<mf::Session> const& session) = 0;

    virtual void add_display(Rectangle const& area) = 0;

    virtual void remove_display(Rectangle const& area) = 0;
};

class FullscreenWindowManager : public WindowManager, me::FullscreenPlacementStrategy
{
public:
    using me::FullscreenPlacementStrategy::FullscreenPlacementStrategy;

private:
    void add_surface(std::shared_ptr<ms::Surface> const&, ms::Session*) override {}

    void remove_surface(std::weak_ptr<ms::Surface> const&) override {}

    void add_session(std::shared_ptr<mf::Session> const&) override {}

    void remove_session(std::shared_ptr<mf::Session> const&) override {}

    void add_display(Rectangle const&) override {}

    void remove_display(Rectangle const&) override {}
};

class TilingWindowManager : public WindowManager
{
    auto place(ms::Session const& session, ms::SurfaceCreationParameters const& request_parameters)
    -> ms::SurfaceCreationParameters override
    {
        auto parameters = request_parameters;

        std::lock_guard<decltype(mutex)> lock(mutex);
        auto const ptile = tiles.find(&session);
        if (ptile != end(tiles))
        {
            Rectangle const& tile = ptile->second;
            parameters.top_left = parameters.top_left + (tile.top_left - Point{0, 0});

            clip_to_tile(parameters, tile);
        }

        return parameters;
    }

    void add_surface(
        std::shared_ptr<ms::Surface> const& surface,
        ms::Session* session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        surfaces.emplace(session, surface);
    }

    void remove_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
        // This looks odd but we want to block in case we're using the surface
        std::lock_guard<decltype(mutex)> lock(mutex);
    }

    void add_session(std::shared_ptr<mf::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        tiles[session.get()] = Rectangle{};
        update_tiles();
    }

    void remove_session(std::shared_ptr<mf::Session> const& session)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        tiles.erase(session.get());
        surfaces.erase(session.get());
        update_tiles();
    }

    void add_display(Rectangle const& area)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        displays.push_back(area);
        update_tiles();
    }

    void remove_display(Rectangle const& area)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        auto const i = std::find(begin(displays), end(displays), area);
        if (i != end(displays)) displays.erase(i);
        update_tiles();
    }

private:
    void update_tiles()
    {
        if (tiles.size() < 1 || displays.size() < 1) return;

        auto const sessions = tiles.size();
        Rectangles view;

        for (auto const& display : displays)
            view.add(display);

        auto const bounding_rect = view.bounding_rectangle();

        auto const total_width  = bounding_rect.size.width.as_int();
        auto const total_height = bounding_rect.size.height.as_int();

        auto index = 0;

        for (auto& tile : tiles)
        {
            auto const x = (total_width*index)/sessions;
            ++index;
            auto const dx = (total_width*index)/sessions - x;

            auto const old_tile = tile.second;
            Rectangle const new_tile{{x, 0}, {dx, total_height}};

            update_surfaces(tile.first, old_tile, new_tile);

            tile.second = new_tile;
        }
    }

    void update_surfaces(mf::Session const* session, Rectangle const& old_tile, Rectangle const& new_tile)
    {
        auto displacement = new_tile.top_left - old_tile.top_left;
        auto const moved = surfaces.equal_range(session);

        for (auto p = moved.first; p != moved.second; ++p)
        {
            if (auto const surface = p->second.lock())
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

    std::mutex mutex;
    std::vector<Rectangle> displays;
    std::map<mf::Session const*, Rectangle> tiles;
    std::multimap<mf::Session const*, std::weak_ptr<ms::Surface>> surfaces;
};

auto const option = "window-manager";
auto const description = "window management strategy [{tiling|fullscreen}]";
auto const tiling = "tiling";
auto const fullscreen = "fullscreen";

class WindowManagmentFactory
{
public:
    explicit WindowManagmentFactory(mir::Server& server) : server{server} {}

    auto window_manager() -> std::shared_ptr<WindowManager>
    {
        auto tmp = wm.lock();

        if (!tmp)
        {
            auto const options = server.get_options();
            auto const selection = options->get<std::string>(option);

            if (selection == tiling)
                tmp = std::make_shared<TilingWindowManager>();
            else if (selection == fullscreen)
                tmp = std::make_shared<FullscreenWindowManager>(server.the_shell_display_layout());
            else
                throw mir::AbnormalExit("Unknown window manager: " + selection);

            wm = tmp;
        }

        return tmp;
    }

private:
    mir::Server& server;
    std::weak_ptr<WindowManager> wm;
};

class SessionTracker : public msh::SessionCoordinatorWrapper
{
public:
    SessionTracker(
        std::shared_ptr<ms::SessionCoordinator> const& wrapped,
        std::shared_ptr<WindowManager> const& window_manager) :
        msh::SessionCoordinatorWrapper(wrapped),
        window_manager(window_manager)
    {
    }

private:
    auto open_session(pid_t client_pid, std::string const& name, std::shared_ptr<mf::EventSink> const& sink)
    -> std::shared_ptr<mf::Session> override
    {
        auto const new_session = msh::SessionCoordinatorWrapper::open_session(client_pid, name, sink);
        window_manager->add_session(new_session);
        return new_session;
    }

    void close_session(std::shared_ptr<mf::Session> const& session) override
    {
        window_manager->remove_session(session);
        msh::SessionCoordinatorWrapper::close_session(session);
    }

    std::shared_ptr<WindowManager> const window_manager;
};

class SurfaceTracker : public msh::SurfaceCoordinatorWrapper
{
public:
    SurfaceTracker(
        std::shared_ptr<ms::SurfaceCoordinator> const& wrapped,
        std::shared_ptr<WindowManager> const& window_manager) :
        msh::SurfaceCoordinatorWrapper(wrapped),
        window_manager(window_manager)
    {
    }

private:
    auto add_surface(ms::SurfaceCreationParameters const& params, ms::Session* session)
    -> std::shared_ptr<ms::Surface> override
    {
        auto const new_surface = msh::SurfaceCoordinatorWrapper::add_surface(params, session);
        window_manager->add_surface(new_surface, session);
        return new_surface;
    }

    void remove_surface(std::weak_ptr<ms::Surface> const& surface)
    {
        window_manager->remove_surface(surface);
        msh::SurfaceCoordinatorWrapper::remove_surface(surface);
    }

    std::shared_ptr<WindowManager> const window_manager;
};

class DisplayTracker : public mc::DisplayBufferCompositor
{
public:
    DisplayTracker(
        std::unique_ptr<mc::DisplayBufferCompositor>&& wrapped,
        Rectangle const& area,
        std::shared_ptr<WindowManager> const& window_manager) :
        wrapped{std::move(wrapped)},
        area{area},
        window_manager(window_manager)
    {
        window_manager->add_display(area);
    }

    ~DisplayTracker() noexcept
    {
        window_manager->remove_display(area);
    }

private:

    void composite(mc::SceneElementSequence&& scene_sequence) override
    {
        wrapped->composite(std::move(scene_sequence));
    }

    std::unique_ptr<mc::DisplayBufferCompositor> const wrapped;
    Rectangle const area;
    std::shared_ptr<WindowManager> const window_manager;
};

class DisplayTrackerFactory : public mc::DisplayBufferCompositorFactory
{
public:
    DisplayTrackerFactory(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped,
        std::shared_ptr<WindowManager> const& window_manager) :
        wrapped{wrapped},
        window_manager(window_manager)
    {
    }

private:
    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& display_buffer)
    {
        auto compositor = wrapped->create_compositor_for(display_buffer);
        return std::unique_ptr<mc::DisplayBufferCompositor>{
            new DisplayTracker{std::move(compositor), display_buffer.view_area(), window_manager}};
    }

    std::shared_ptr<mc::DisplayBufferCompositorFactory> const wrapped;
    std::shared_ptr<WindowManager> const window_manager;
};
}

void me::add_window_manager_option_to(Server& server)
{
    server.add_configuration_option(option, description, mir::OptionType::string);

    auto const factory = std::make_shared<WindowManagmentFactory>(server);

    server.override_the_placement_strategy([factory, &server]()
        -> std::shared_ptr<ms::PlacementStrategy>
        {
            auto const options = server.get_options();

            if (!options->is_set(option))
                return std::shared_ptr<ms::PlacementStrategy>{};

            return factory->window_manager();
        });

    server.wrap_session_coordinator([factory, &server]
        (std::shared_ptr<ms::SessionCoordinator> const& wrapped)
        -> std::shared_ptr<ms::SessionCoordinator>
        {
            auto const options = server.get_options();

            if (!options->is_set(option))
                return wrapped;

            return std::make_shared<SessionTracker>(wrapped, factory->window_manager());
        });

    server.wrap_surface_coordinator([factory, &server]
        (std::shared_ptr<ms::SurfaceCoordinator> const& wrapped)
        -> std::shared_ptr<ms::SurfaceCoordinator>
        {
            auto const options = server.get_options();

            if (!options->is_set(option))
                return wrapped;

            return std::make_shared<SurfaceTracker>(wrapped, factory->window_manager());
        });

    server.wrap_display_buffer_compositor_factory([factory, &server]
       (std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
       -> std::shared_ptr<mc::DisplayBufferCompositorFactory>
       {
           auto const options = server.get_options();

           if (!options->is_set(option))
               return wrapped;

           return std::make_shared<DisplayTrackerFactory>(wrapped, factory->window_manager());
       });
}
