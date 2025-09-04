/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/window_management_options.h"

#include "basic_window_manager.h"
#include "window_management_trace.h"

#include <mir/abnormal_exit.h>
#include <mir/server.h>
#include <mir/options/option.h>
#include <mir/shell/system_compositor_window_manager.h>

namespace msh = mir::shell;

// Demonstrate introducing a window management strategy
namespace
{
char const* const wm_option = "window-manager";
char const* const trace_option = "window-management-trace";
}

void miral::WindowManagerOptions::operator()(mir::Server& server) const
{
    std::string description = "window management strategy [{";

    auto first = true;
    for (auto const& option : policies)
    {
        if (!first) description += "|";
        first = false;

        description += option.name;
    }

    description += "}]";

    server.add_configuration_option(wm_option, description, policies.begin()->name);
    server.add_configuration_option(trace_option, "Log trace message", mir::OptionType::null);

    server.override_the_window_manager_builder([this, &server](msh::FocusController* focus_controller)
        -> std::shared_ptr<msh::WindowManager>
        {
            auto const options = server.get_options();
            auto const selection = options->get<std::string>(wm_option);

            auto const display_layout = server.the_shell_display_layout();
            auto const persistent_surface_store = server.the_persistent_surface_store();
            auto const input_device_registry = server.the_input_device_registry();

            for (auto const& option : policies)
            {
                if (selection == option.name)
                {
                    if (server.get_options()->is_set(trace_option))
                    {
                        auto trace_builder = [&option](WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
                            {
                                return std::make_unique<WindowManagementTrace>(tools, option.build);
                            };

                        return std::make_shared<BasicWindowManager>(
                            focus_controller,
                            display_layout,
                            persistent_surface_store,
                            *server.the_display_configuration_observer_registrar(),
                            input_device_registry,
                            trace_builder);
                    }

                    return std::make_shared<BasicWindowManager>
                        (focus_controller,
                         display_layout,
                         persistent_surface_store,
                         *server.the_display_configuration_observer_registrar(),
                         input_device_registry,
                         option.build);
                }
            }

            throw mir::AbnormalExit("Unknown window manager: " + selection);
        });
}
