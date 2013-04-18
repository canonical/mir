/*
 * Copyright © 2013 Canonical Ltd.
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

#include "application_switcher.h"
#include "fullscreen_placement_strategy.h"

#include "mir/run_mir.h"
#include "mir/default_server_configuration.h"
#include "mir/shell/session_manager.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/single_visibility_focus_mechanism.h"
#include "mir/shell/session_container.h"
#include "mir/shell/organising_surface_factory.h"
#include "mir/graphics/display.h"

#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

namespace me = mir::examples;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mi = mir::input;

namespace mir
{
namespace examples
{

struct DemoServerConfiguration : mir::DefaultServerConfiguration
{
    DemoServerConfiguration(int argc, char const* argv[])
      : DefaultServerConfiguration(argc, argv)
    {
    }

    std::shared_ptr<msh::PlacementStrategy> the_shell_placement_strategy()
    {
        return shell_placement_strategy(
            [this]
            {
                return std::make_shared<me::FullscreenPlacementStrategy>(the_display());
            });
    }

    std::initializer_list<std::shared_ptr<mi::EventFilter> const> the_event_filters() override
    {
        if (!app_switcher)
            app_switcher = std::make_shared<me::ApplicationSwitcher>();
        return std::initializer_list<std::shared_ptr<mi::EventFilter> const>{app_switcher};
    }

    std::shared_ptr<me::ApplicationSwitcher> app_switcher;
};

}
}

int main(int argc, char const* argv[])
try
{
    me::DemoServerConfiguration config(argc, argv);
    mir::run_mir(config, [&config](mir::DisplayServer&)
        {
            // We use this strange two stage initialization to avoid a circular dependency between the EventFilters
            // and the SessionStore
            config.app_switcher->set_focus_controller(config.the_focus_controller());
        });
    return 0;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << boost::diagnostic_information(error) << std::endl;
    return 1;
} 
