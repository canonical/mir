/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/server.h"

#include "example_display_configuration_policy.h"

#include "mir/input/composite_event_filter.h"
#include "mir/options/option.h"

#include "graphics.h"

#include <linux/input.h>

namespace me = mir::examples;
namespace mg = mir::graphics;

namespace
{
class QuitFilter : public mir::input::EventFilter
{
public:
    QuitFilter(mir::Server& server)
        : server{server}
    {
    }

    bool handle(MirEvent const& event) override
    {
        if (event.type == mir_event_type_key &&
            event.key.action == mir_key_action_down &&
            (event.key.modifiers & mir_key_modifier_alt) &&
            (event.key.modifiers & mir_key_modifier_ctrl) &&
            event.key.scan_code == KEY_BACKSPACE)
        {
            server.stop();
            return true;
        }

        return false;
    }

private:
    mir::Server& server;
};
}

int main(int argc, char const* argv[])
{
    static char const* const launch_child_opt = "launch-client";
    static char const* const launch_client_descr = "system() command to launch client";

    mir::Server server;

    auto const quit_filter = std::make_shared<QuitFilter>(server);
    server.add_configuration_option(
        launch_child_opt, launch_client_descr, mir::OptionType::string)(
        me::display_config_opt, me::display_config_descr, me::clone_opt_val);

    server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto display_config = server.get_options()->get<std::string>(me::display_config_opt);

            if (display_config == me::sidebyside_opt_val)
                return std::make_shared<me::SideBySideDisplayConfigurationPolicy>();
            else if (display_config == me::single_opt_val)
                return std::make_shared<me::SingleDisplayConfigurationPolicy>();
            else
                return wrapped;
        });

    server.set_command_line(argc, argv);
    server.set_init_callback([&]
        {
            server.the_composite_event_filter()->append(quit_filter);

            auto const options = server.get_options();

            if (options->is_set(launch_child_opt))
            {
                auto ignore = std::system((options->get<std::string>(launch_child_opt) + "&").c_str());
                (void)ignore;
            }
        });

    server.run();

    return server.exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
