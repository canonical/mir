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

#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/input/composite_event_filter.h"
#include "mir/options/option.h"

#include "graphics.h"

#include <linux/input.h>

#include <unordered_map>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
class SideBySideDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    void apply_to(mg::DisplayConfiguration& conf)
    {
        size_t const preferred_mode_index{0};
        int max_x = 0;
        std::unordered_map<mg::DisplayConfigurationCardId, size_t> available_outputs_for_card;

        conf.for_each_card(
            [&](mg::DisplayConfigurationCard const& card)
            {
                available_outputs_for_card[card.id] = card.max_simultaneous_outputs;
            });

        conf.for_each_output(
            [&](mg::UserDisplayConfigurationOutput& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0 &&
                    available_outputs_for_card[conf_output.card_id] > 0)
                {
                    conf_output.used = true;
                    conf_output.top_left = geom::Point{max_x, 0};
                    conf_output.current_mode_index = preferred_mode_index;
                    conf_output.power_mode = mir_power_mode_on;
                    conf_output.orientation = mir_orientation_normal;
                    max_x += conf_output.modes[preferred_mode_index].size.width.as_int();
                    --available_outputs_for_card[conf_output.card_id];
                }
                else
                {
                    conf_output.used = false;
                    conf_output.power_mode = mir_power_mode_off;
                }
            });
    }
};

class SingleDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    void apply_to(mg::DisplayConfiguration& conf)
    {
        size_t const preferred_mode_index{0};
        bool done{false};

        conf.for_each_output(
            [&](mg::UserDisplayConfigurationOutput& conf_output)
            {
                if (!done && conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf_output.used = true;
                    conf_output.top_left = geom::Point{0, 0};
                    conf_output.current_mode_index = preferred_mode_index;
                    conf_output.power_mode = mir_power_mode_on;
                    done = true;
                }
                else
                {
                    conf_output.used = false;
                    conf_output.power_mode = mir_power_mode_off;
                }
            });
    }
};

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
    static char const* const display_config_opt = "display-config";
    static char const* const clone_opt_val = "clone";
    static char const* const sidebyside_opt_val = "sidebyside";
    static char const* const single_opt_val = "single";

    mir::Server server;

    auto const quit_filter = std::make_shared<QuitFilter>(server);

    server.add_configuration_option(
        launch_child_opt, "system() command to launch client", mir::OptionType::string)
       (display_config_opt, "Display configuration [{clone,sidebyside,single}]", clone_opt_val);

    server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto display_config = server.get_options()->get<std::string>(display_config_opt);

            if (display_config == sidebyside_opt_val)
                return std::make_shared<SideBySideDisplayConfigurationPolicy>();
            else if (display_config == single_opt_val)
                return std::make_shared<SingleDisplayConfigurationPolicy>();
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
