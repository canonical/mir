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
      : DefaultServerConfiguration(argc, argv),
        app_switcher(std::make_shared<me::ApplicationSwitcher>())
    {
    }
    
    std::shared_ptr<mf::Shell> the_frontend_shell() override
    {
        return session_manager(
            [this]() -> std::shared_ptr<msh::SessionManager>
            {
                auto session_container = std::make_shared<msh::SessionContainer>();
                auto focus_mechanism = std::make_shared<msh::SingleVisibilityFocusMechanism>(session_container, the_input_focus_selector());
                auto focus_selection_strategy = std::make_shared<msh::RegistrationOrderFocusSequence>(session_container);

                auto placement_strategy = std::make_shared<me::FullscreenPlacementStrategy>(the_display());
                auto organising_factory = std::make_shared<msh::OrganisingSurfaceFactory>(the_surface_factory(), placement_strategy);

                return std::make_shared<msh::SessionManager>(organising_factory, session_container,
                    focus_selection_strategy, focus_mechanism);
        });
    }
    

    std::initializer_list<std::shared_ptr<mi::EventFilter> const> the_event_filters() override
    {
        static std::initializer_list<std::shared_ptr<mi::EventFilter> const> filter_list = { app_switcher };
        return filter_list;
    }

    std::shared_ptr<me::ApplicationSwitcher> app_switcher;
};

}
}

int main(int argc, char const* argv[])
try
{
    me::DemoServerConfiguration config(argc, argv);
    mir::run_mir(config, [&config](mir::DisplayServer&){
            // TODO: Remove strange dynamic cast. Perhaps this is an indication that SessionManager provides an interface such as
            // "FocusController" (and configuration, the_focus_controller())
            // TODO: We use this strange two phase initialization in order to
            // avoid a circular dependency between the_input_filters() and the_session_store(). This seems to indicate that perhaps
            // the InputManager should not be the InputFocusSelector
            config.app_switcher->set_focus_controller(std::dynamic_pointer_cast<msh::SessionManager>(config.the_frontend_shell()));
        });
    return 0;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << boost::diagnostic_information(error) << std::endl;
    return 1;
} 
