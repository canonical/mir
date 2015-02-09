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
#include "server_example_window_management.h"

#include "mir/server.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/graphics/display_buffer.h"
#include "mir/options/option.h"
#include "mir/scene/session_listener.h"

namespace mc = mir::compositor;
namespace me = mir::examples;
namespace mg = mir::graphics;
namespace ms = mir::scene;
using namespace mir::geometry;

///\example server_example_window_manager.cpp
/// Demonstrate introducing a window management strategy

namespace
{
class SceneTracker : public ms::SessionListener
{
public:
    SceneTracker(std::shared_ptr<me::WindowManager> const& window_manager) :
        window_manager(window_manager)
    {
    }

private:

    void starting(std::shared_ptr<ms::Session> const& session) override
    {
        window_manager->add_session(session);
    }

    void stopping(std::shared_ptr<ms::Session> const& session) override
    {
        window_manager->remove_session(session);
    }

    void focused(std::shared_ptr<ms::Session> const& /*session*/) override {}
    void unfocused() override {}

    void surface_created(ms::Session& session, std::shared_ptr<ms::Surface> const& surface) override
    {
        window_manager->add_surface(surface, &session);
    }

    void destroying_surface(ms::Session& session, std::shared_ptr<ms::Surface> const& surface) override
    {
        window_manager->remove_surface(surface, &session);
    }

    std::shared_ptr<me::WindowManager> const window_manager;
};

class DisplayTracker : public mc::DisplayBufferCompositor
{
public:
    DisplayTracker(
        std::unique_ptr<mc::DisplayBufferCompositor>&& wrapped,
        Rectangle const& area,
        std::shared_ptr<me::WindowManager> const& window_manager) :
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
    std::shared_ptr<me::WindowManager> const window_manager;
};

class DisplayTrackerFactory : public mc::DisplayBufferCompositorFactory
{
public:
    DisplayTrackerFactory(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped,
        std::shared_ptr<me::WindowManager> const& window_manager) :
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
    std::shared_ptr<me::WindowManager> const window_manager;
};
}

void me::add_window_manager_option_to(Server& server)
{
    server.add_configuration_option(me::wm_option, me::wm_description, mir::OptionType::string);

    auto const factory = std::make_shared<me::WindowManagmentFactory>(server);

    server.override_the_placement_strategy([factory, &server]()
        -> std::shared_ptr<ms::PlacementStrategy>
        {
            auto const options = server.get_options();

            if (!options->is_set(me::wm_option))
                return std::shared_ptr<ms::PlacementStrategy>{};

            return factory->window_manager();
        });

    server.override_the_session_listener([factory, &server]()
        -> std::shared_ptr<ms::SessionListener>
        {
            auto const options = server.get_options();

            if (!options->is_set(me::wm_option))
                return std::shared_ptr<ms::SessionListener>{};

            return std::make_shared<SceneTracker>(factory->window_manager());
        });

    server.override_the_surface_configurator([factory, &server]()
        -> std::shared_ptr<ms::SurfaceConfigurator>
        {
            auto const options = server.get_options();

            if (!options->is_set(me::wm_option))
                return std::shared_ptr<ms::SurfaceConfigurator>{};

            return factory->window_manager();
        });

    server.wrap_display_buffer_compositor_factory([factory, &server]
       (std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
       -> std::shared_ptr<mc::DisplayBufferCompositorFactory>
       {
           auto const options = server.get_options();

           if (!options->is_set(me::wm_option))
               return wrapped;

           return std::make_shared<DisplayTrackerFactory>(wrapped, factory->window_manager());
       });
}
