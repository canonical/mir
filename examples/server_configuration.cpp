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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "server_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/input/composite_event_filter.h"
#include "mir/main_loop.h"

#include <string>

#include <linux/input.h>
#include <unordered_map>

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

char const* const display_config_opt = "display-config";
char const* const clone_opt_val = "clone";
char const* const sidebyside_opt_val = "sidebyside";
char const* const single_opt_val = "single";

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
            [&](mg::DisplayConfigurationOutput const& conf_output)
            {
                if (conf_output.connected && conf_output.modes.size() > 0 &&
                    available_outputs_for_card[conf_output.card_id] > 0)
                {
                    conf.configure_output(conf_output.id, true,
                                          geom::Point{max_x, 0},
                                          preferred_mode_index,
                                          conf_output.current_format,
                                          mir_power_mode_on,
                                          mir_orientation_normal);
                    max_x += conf_output.modes[preferred_mode_index].size.width.as_int();
                    --available_outputs_for_card[conf_output.card_id];
                }
                else
                {
                    conf.configure_output(conf_output.id, false,
                                          conf_output.top_left,
                                          conf_output.current_mode_index,
                                          conf_output.current_format,
                                          mir_power_mode_on,
                                          mir_orientation_normal);
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
            [&](mg::DisplayConfigurationOutput const& conf_output)
            {
                if (!done && conf_output.connected && conf_output.modes.size() > 0)
                {
                    conf.configure_output(conf_output.id, true,
                                          geom::Point{0, 0},
                                          preferred_mode_index,
                                          conf_output.current_format,
                                          mir_power_mode_on,
                                          mir_orientation_normal);
                    done = true;
                }
                else
                {
                    conf.configure_output(conf_output.id, false,
                                          conf_output.top_left,
                                          conf_output.current_mode_index,
                                          conf_output.current_format,
                                          mir_power_mode_on,
                                          mir_orientation_normal);
                }
            });
    }
};

class QuitFilter : public mir::input::EventFilter
{
public:
    QuitFilter(std::shared_ptr<mir::MainLoop> const& main_loop)
        : main_loop{main_loop}
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
            main_loop->stop();
            return true;
        }

        return false;
    }

private:
    std::shared_ptr<mir::MainLoop> const main_loop;
};

}

me::ServerConfiguration::ServerConfiguration(int argc, char const** argv)
    : DefaultServerConfiguration(argc, argv)
{
    namespace po = boost::program_options;

    add_options()
        (display_config_opt, po::value<std::string>()->default_value(clone_opt_val),
            "Display configuration [{clone,sidebyside,single}]");
}

std::shared_ptr<mg::DisplayConfigurationPolicy>
me::ServerConfiguration::the_display_configuration_policy()
{
    return display_configuration_policy(
        [this]() -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto display_config = the_options()->get<std::string>(display_config_opt);

            if (display_config == sidebyside_opt_val)
                return std::make_shared<SideBySideDisplayConfigurationPolicy>();
            else if (display_config == single_opt_val)
                return std::make_shared<SingleDisplayConfigurationPolicy>();
            else
                return DefaultServerConfiguration::the_display_configuration_policy();
        });
}

std::shared_ptr<mir::input::CompositeEventFilter>
me::ServerConfiguration::the_composite_event_filter()
{
    if (!quit_filter)
        quit_filter = std::make_shared<QuitFilter>(the_main_loop());

    auto composite_filter = DefaultServerConfiguration::the_composite_event_filter();
    composite_filter->append(quit_filter);

    return composite_filter;
}
