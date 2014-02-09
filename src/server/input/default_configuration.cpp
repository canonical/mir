/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/default_server_configuration.h"

#include "display_input_region.h"
#include "event_filter_chain.h"
#include "nested_input_configuration.h"
#include "nested_input_relay.h"
#include "null_input_configuration.h"

#include "mir/input/android/default_android_input_configuration.h"
#include "mir/report/legacy_input_report.h"


namespace mi = mir::input;
namespace mia = mi::android;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace msh = mir::shell;

std::shared_ptr<mi::InputRegion> mir::DefaultServerConfiguration::the_input_region()
{
    return input_region(
        [this]()
        {
            return std::make_shared<mi::DisplayInputRegion>(the_display());
        });
}

std::shared_ptr<mi::CompositeEventFilter>
mir::DefaultServerConfiguration::the_composite_event_filter()
{
    return composite_event_filter(
        [this]() -> std::shared_ptr<mi::CompositeEventFilter>
        {
            std::initializer_list<std::shared_ptr<mi::EventFilter> const> filter_list {default_filter};
            return std::make_shared<mi::EventFilterChain>(filter_list);
        });
}

std::shared_ptr<mi::InputConfiguration>
mir::DefaultServerConfiguration::the_input_configuration()
{
    return input_configuration(
    [this]() -> std::shared_ptr<mi::InputConfiguration>
    {
        auto const options = the_options();
        if (!options->get<bool>(enable_input_opt))
        {
            return std::make_shared<mi::NullInputConfiguration>();
        }
        // TODO (default-nested): don't fallback to standalone if host socket is unset in 14.04
        else if (options->is_set(standalone_opt) || !options->is_set(host_socket_opt))
        {
            return std::make_shared<mia::DefaultInputConfiguration>(
                the_composite_event_filter(),
                the_input_region(),
                the_cursor_listener(),
                the_input_report());
        }
        else
        {
            return std::make_shared<mi::NestedInputConfiguration>(
                the_nested_input_relay(),
                the_composite_event_filter(),
                the_input_region(),
                the_cursor_listener(),
                the_input_report());
        }
    });
}

std::shared_ptr<mi::InputManager>
mir::DefaultServerConfiguration::the_input_manager()
{
    return input_manager(
        [&, this]() -> std::shared_ptr<mi::InputManager>
        {
            if (the_options()->get<std::string>(legacy_input_report_opt) == log_opt_value)
                    mr::legacy_input::initialize(the_logger());
            return the_input_configuration()->the_input_manager();
        });
}

std::shared_ptr<msh::InputTargeter> mir::DefaultServerConfiguration::the_input_targeter()
{
    return input_targeter(
        [&]() -> std::shared_ptr<msh::InputTargeter>
        {
            return the_input_configuration()->the_input_targeter();
        });
}

std::shared_ptr<ms::InputRegistrar> mir::DefaultServerConfiguration::the_input_registrar()
{
    return input_registrar(
        [&]() -> std::shared_ptr<ms::InputRegistrar>
        {
            return the_input_configuration()->the_input_registrar();
        });
}

auto mir::DefaultServerConfiguration::the_nested_input_relay()
-> std::shared_ptr<mi::NestedInputRelay>
{
    return nested_input_relay([]{ return std::make_shared<mi::NestedInputRelay>(); });
}

auto mir::DefaultServerConfiguration::the_nested_event_filter()
-> std::shared_ptr<mi::EventFilter>
{
    return the_nested_input_relay();
}

