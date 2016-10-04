/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "display_configuration_observer_multiplexer.h"

namespace mg = mir::graphics;

void mg::DisplayConfigurationObserverMultiplexer::initial_configuration(
    mir::graphics::DisplayConfiguration const& config)
{
    for_each_observer([&config](auto& observer) { observer.initial_configuration(config); });
}

void mg::DisplayConfigurationObserverMultiplexer::configuration_applied(
    mir::graphics::DisplayConfiguration const& config)
{
    for_each_observer([&config](auto& observer) { observer.configuration_applied(config); });
}

void mg::DisplayConfigurationObserverMultiplexer::configuration_failed(
    mir::graphics::DisplayConfiguration const& attempted,
    std::exception const& error)
{
    for_each_observer(
        [&attempted, &error](auto& observer)
        {
            observer.configuration_failed(attempted, error);
        });
}

void mg::DisplayConfigurationObserverMultiplexer::catastrophic_configuration_error(
    mg::DisplayConfiguration const& failed_fallback,
    std::exception const& error)
{
    for_each_observer(
        [&failed_fallback, &error](auto& observer)
        {
            observer.catastrophic_configuration_error(failed_fallback, error);
        });
}
