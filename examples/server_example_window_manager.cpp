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

#include "mir/server.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/graphics/display_buffer.h"
#include "mir/options/option.h"
#include "mir/scene/observer.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/session_coordinator_wrapper.h"
#include "mir/shell/surface_coordinator_wrapper.h"

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
class WindowManager : public ms::PlacementStrategy
{
public:
    auto place(ms::Session const&, ms::SurfaceCreationParameters const& request_parameters)
    -> ms::SurfaceCreationParameters override
    {
        return request_parameters;
    }

    void add_surface(
        std::shared_ptr<ms::Surface> const& /*new_surface*/,
        ms::Session* /*session*/)
    {
    }

    void remove_surface(std::weak_ptr<ms::Surface> const& /*surface*/)
    {
    }

    void add_session(std::shared_ptr<mf::Session> const& /*session*/)
    {
    }

    void remove_session(std::shared_ptr<mf::Session> const& /*session*/)
    {
    }

    void add_display(Rectangle const& /*area*/)
    {
    }

    void remove_display(Rectangle const& /*area*/)
    {
    }

private:
};

class WindowManagmentFactory
{
public:
    auto make_tiling_wm() -> std::shared_ptr<WindowManager>
    {
        auto tmp = wm.lock();

        if (!tmp)
        {
            tmp = std::make_shared<WindowManager>();
            wm = tmp;
        }

        return tmp;
    }

private:
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
    static auto const option = "window-manager";
    static auto const description = "window management strategy [{tiling}]";
    static auto const tiling = "tiling";

    server.add_configuration_option(option, description, mir::OptionType::string);

    auto const factory = std::make_shared<WindowManagmentFactory>();

    server.override_the_placement_strategy([factory, &server]()
        -> std::shared_ptr<ms::PlacementStrategy>
        {
            auto const options = server.get_options();

            if (!options->is_set(option))
                return std::shared_ptr<ms::PlacementStrategy>{};

            if (options->get<std::string>(option) == tiling)
                return factory->make_tiling_wm();

            BOOST_THROW_EXCEPTION(std::runtime_error("Unknown window manager: " + options->get<std::string>(option)));
        });

    server.wrap_session_coordinator([factory, &server]
        (std::shared_ptr<ms::SessionCoordinator> const& wrapped)
        -> std::shared_ptr<ms::SessionCoordinator>
        {
            auto const options = server.get_options();

            if (!options->is_set(option))
                return wrapped;

            if (options->get<std::string>(option) == tiling)
                return std::make_shared<SessionTracker>(wrapped, factory->make_tiling_wm());

            BOOST_THROW_EXCEPTION(std::runtime_error("Unknown window manager: " + options->get<std::string>(option)));
        });

    server.wrap_surface_coordinator([factory, &server]
        (std::shared_ptr<ms::SurfaceCoordinator> const& wrapped)
        -> std::shared_ptr<ms::SurfaceCoordinator>
        {
            auto const options = server.get_options();

            if (!options->is_set(option))
                return wrapped;

            if (options->get<std::string>(option) == tiling)
                return std::make_shared<SurfaceTracker>(wrapped, factory->make_tiling_wm());

            BOOST_THROW_EXCEPTION(std::runtime_error("Unknown window manager: " + options->get<std::string>(option)));
        });


}
