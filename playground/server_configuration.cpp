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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "server_configuration.h"
#include "mir/options/default_configuration.h"
#include "mir/input/composite_event_filter.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/main_loop.h"

#include "server_example_display_configuration_policy.h"
#include "server_example_input_event_filter.h"

namespace me = mir::examples;
namespace mg = mir::graphics;

char const* const me::wm_option = "window-manager";
char const* const me::wm_description = "window management strategy [{legacy|canonical|tiling}]";
char const* const me::wm_tiling = "tiling";
char const* const me::wm_legacy = "legacy";
char const* const me::wm_canonical = "canonical";

me::ServerConfiguration::ServerConfiguration(std::shared_ptr<options::DefaultConfiguration> const& configuration_options) :
    DefaultServerConfiguration(configuration_options)
{
    namespace po = boost::program_options;

    configuration_options->add_options()
        (me::display_config_opt, po::value<std::string>()->default_value(me::clone_opt_val),
            me::display_config_descr);
    configuration_options->add_options()
        (wm_option, po::value<std::string>()->default_value(wm_legacy), wm_description);
}

me::ServerConfiguration::ServerConfiguration(int argc, char const** argv) :
    ServerConfiguration(std::make_shared<options::DefaultConfiguration>(argc, argv))
{
}

std::shared_ptr<mg::DisplayConfigurationPolicy>
me::ServerConfiguration::the_display_configuration_policy()
{
    return display_configuration_policy(
        [this]() -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto display_config = the_options()->get<std::string>(me::display_config_opt);

            if (display_config == me::sidebyside_opt_val)
                return std::make_shared<mg::SideBySideDisplayConfigurationPolicy>();
            else if (display_config == me::single_opt_val)
                return std::make_shared<mg::SingleDisplayConfigurationPolicy>();
            else
                return DefaultServerConfiguration::the_display_configuration_policy();
        });
}

std::shared_ptr<mir::input::CompositeEventFilter>
me::ServerConfiguration::the_composite_event_filter()
{
    return composite_event_filter(
        [this]() -> std::shared_ptr<mir::input::CompositeEventFilter>
        {
            if (!quit_filter)
                quit_filter = std::make_shared<me::QuitFilter>([this] { the_main_loop()->stop(); });

            auto composite_filter = DefaultServerConfiguration::the_composite_event_filter();
            composite_filter->append(quit_filter);

            return composite_filter;
        });
}
