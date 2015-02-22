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
#include "mir/main_loop.h"

#include "server_example_display_configuration_policy.h"
#include "server_example_input_event_filter.h"

namespace me = mir::examples;
namespace mg = mir::graphics;

me::ServerConfiguration::ServerConfiguration(std::shared_ptr<options::DefaultConfiguration> const& configuration_options) :
    DefaultServerConfiguration(configuration_options)
{
    namespace po = boost::program_options;

    configuration_options->add_options()
        (me::display_config_opt, po::value<std::string>()->default_value(me::clone_opt_val),
            me::display_config_descr);
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
                return std::make_shared<SideBySideDisplayConfigurationPolicy>();
            else if (display_config == me::single_opt_val)
                return std::make_shared<SingleDisplayConfigurationPolicy>();
            else
                return DefaultServerConfiguration::the_display_configuration_policy();
        });
}

std::shared_ptr<mir::input::CompositeEventFilter>
me::ServerConfiguration::the_composite_event_filter()
{
    if (!quit_filter)
        quit_filter = std::make_shared<me::QuitFilter>([this] { the_main_loop()->stop(); });

    auto composite_filter = DefaultServerConfiguration::the_composite_event_filter();
    composite_filter->append(quit_filter);

    return composite_filter;
}
