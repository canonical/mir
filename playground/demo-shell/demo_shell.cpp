/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

/// \example demo_shell.cpp A simple mir shell

#include "demo_compositor.h"
#include "window_manager.h"
#include "fullscreen_placement_strategy.h"
#include "../server_configuration.h"

#include "mir/options/default_configuration.h"
#include "mir/run_mir.h"
#include "mir/report_exception.h"
#include "mir/graphics/display.h"
#include "mir/input/composite_event_filter.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/destination_alpha.h"
#include "mir/compositor/renderer_factory.h"
#include "mir/shell/host_lifecycle_event_listener.h"

#include <iostream>

namespace me = mir::examples;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mo = mir::options;
namespace mc = mir::compositor;
namespace msh = mir::shell;

namespace mir
{
namespace examples
{

class DisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    DisplayBufferCompositorFactory(
        std::shared_ptr<mg::GLProgramFactory> const& gl_program_factory,
        std::shared_ptr<mc::CompositorReport> const& report) :
        gl_program_factory(gl_program_factory),
        report(report)
    {
    }

    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(
        mg::DisplayBuffer& display_buffer) override
    {
        return std::unique_ptr<mc::DisplayBufferCompositor>(
            new me::DemoCompositor{display_buffer, *gl_program_factory, report});
    }

private:
    std::shared_ptr<mg::GLProgramFactory> const gl_program_factory;
    std::shared_ptr<mc::CompositorReport> const report;
};

class DemoServerConfiguration : public mir::examples::ServerConfiguration
{
public:
    DemoServerConfiguration(int argc, char const* argv[],
                            std::initializer_list<std::shared_ptr<mi::EventFilter>> const& filter_list)
      : ServerConfiguration([argc, argv]
        {
            auto result = std::make_shared<mo::DefaultConfiguration>(argc, argv);

            namespace po = boost::program_options;

            result->add_options()
                ("fullscreen-surfaces", "Make all surfaces fullscreen");

            return result;
        }()),
        filter_list(filter_list)
    {
    }


    std::shared_ptr<compositor::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory() override
    {
        return display_buffer_compositor_factory(
            [this]()
            {
                return std::make_shared<me::DisplayBufferCompositorFactory>(
                    the_gl_program_factory(),
                    the_compositor_report());
            });
    }

    std::shared_ptr<ms::PlacementStrategy> the_placement_strategy() override
    {
        return shell_placement_strategy(
            [this]() -> std::shared_ptr<ms::PlacementStrategy>
            {
                if (the_options()->is_set("fullscreen-surfaces"))
                    return std::make_shared<me::FullscreenPlacementStrategy>(the_shell_display_layout());
                else
                    return DefaultServerConfiguration::the_placement_strategy();
            });
    }

    std::shared_ptr<mi::CompositeEventFilter> the_composite_event_filter() override
    {
        auto composite_filter = ServerConfiguration::the_composite_event_filter();
        for (auto const& filter : filter_list)
            composite_filter->append(filter);

        return composite_filter;
    }

    class NestedLifecycleEventListener : public msh::HostLifecycleEventListener
    {
    public:
        virtual void lifecycle_event_occurred(MirLifecycleState state) override
        {
            printf("Lifecycle event occurred : state = %d\n", state);
        }
    };

    std::shared_ptr<msh::HostLifecycleEventListener> the_host_lifecycle_event_listener() override
    {
       return host_lifecycle_event_listener(
           []()
           {
               return std::make_shared<NestedLifecycleEventListener>();
           });
    }

private:
    std::vector<std::shared_ptr<mi::EventFilter>> const filter_list;
};

}
}

int main(int argc, char const* argv[])
try
{
    auto wm = std::make_shared<me::WindowManager>();
    me::DemoServerConfiguration config(argc, argv, {wm});

    mir::run_mir(config, [&config, &wm](mir::DisplayServer&)
        {
            // We use this strange two stage initialization to avoid a circular dependency between the EventFilters
            // and the SessionStore
            wm->set_focus_controller(config.the_focus_controller());
            wm->set_display(config.the_display());
            wm->set_compositor(config.the_compositor());
            wm->set_input_scene(config.the_input_scene());
        });
    return 0;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return 1;
}
