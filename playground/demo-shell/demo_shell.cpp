/*
 * Copyright © 2013-2015 Canonical Ltd.
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
#include "../server_configuration.h"

#include "mir/run_mir.h"
#include "mir/report_exception.h"
#include "mir/graphics/display.h"
#include "mir/input/composite_event_filter.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/renderer_factory.h"
#include "mir/options/option.h"
#include "default_window_manager.h"
#include "server_example_tiling_window_manager.h"
#include "mir/shell/canonical_window_manager.h"
#include "server_example_host_lifecycle_event_listener.h"

#include <iostream>

namespace me = mir::examples;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mc = mir::compositor;
namespace msh = mir::shell;

namespace mir
{
namespace examples
{
using CanonicalWindowManager = msh::BasicWindowManager<msh::CanonicalWindowManagerPolicy, msh::CanonicalSessionInfo, msh::CanonicalSurfaceInfo>;
using TilingWindowManager = me::BasicWindowManagerCopy<me::TilingWindowManagerPolicy, me::TilingSessionInfo, me::TilingSurfaceInfo>;


class DisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    DisplayBufferCompositorFactory(
        std::shared_ptr<mc::CompositorReport> const& report) :
        report(report)
    {
    }

    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(
        mg::DisplayBuffer& display_buffer) override
    {
        return std::unique_ptr<mc::DisplayBufferCompositor>(
            new me::DemoCompositor{display_buffer, report});
    }

private:
    std::shared_ptr<mc::CompositorReport> const report;
};

class DemoServerConfiguration : public mir::examples::ServerConfiguration
{
public:
    DemoServerConfiguration(int argc, char const* argv[],
                            std::initializer_list<std::shared_ptr<mi::EventFilter>> const& filter_list)
      : ServerConfiguration(argc, argv),
        filter_list(filter_list)
    {
    }


    std::shared_ptr<compositor::DisplayBufferCompositorFactory> the_display_buffer_compositor_factory() override
    {
        return display_buffer_compositor_factory(
            [this]()
            {
                return std::make_shared<me::DisplayBufferCompositorFactory>(
                    the_compositor_report());
            });
    }

    std::shared_ptr<mi::CompositeEventFilter> the_composite_event_filter() override
    {
        return composite_event_filter(
            [this]() -> std::shared_ptr<mi::CompositeEventFilter>
            {
                auto composite_filter = ServerConfiguration::the_composite_event_filter();
                for (auto const& filter : filter_list)
                    composite_filter->append(filter);

                return composite_filter;
            });
    }

    auto the_window_manager_builder() -> shell::WindowManagerBuilder override
    {
        return [this](shell::FocusController* focus_controller)
            -> std::shared_ptr<msh::WindowManager>
            {
                auto const options = the_options();
                auto const selection = options->get<std::string>(wm_option);

                if (selection == wm_tiling)
                {
                    return std::make_shared<TilingWindowManager>(focus_controller);
                }
                else if (selection == wm_canonical)
                {
                    return std::make_shared<CanonicalWindowManager>(
                        focus_controller,
                        the_shell_display_layout());
                }

                return std::make_shared<DefaultWindowManager>(
                    focus_controller,
                    the_shell_display_layout(),
                    the_session_coordinator());
            };
    }

    std::shared_ptr<msh::HostLifecycleEventListener> the_host_lifecycle_event_listener() override
    {
       return host_lifecycle_event_listener(
           [this]()
           {
               return std::make_shared<HostLifecycleEventListener>(the_logger());
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
