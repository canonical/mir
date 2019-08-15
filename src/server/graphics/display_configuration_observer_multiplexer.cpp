/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
namespace ms = mir::scene;

void mg::DisplayConfigurationObserverMultiplexer::initial_configuration(
    std::shared_ptr<mg::DisplayConfiguration const> const& config)
{
    for_each_observer(&mg::DisplayConfigurationObserver::initial_configuration, config);
}

void mg::DisplayConfigurationObserverMultiplexer::configuration_applied(
    std::shared_ptr<mg::DisplayConfiguration const> const& config)
{
    for_each_observer(&mg::DisplayConfigurationObserver::configuration_applied, config);
}

void mg::DisplayConfigurationObserverMultiplexer::base_configuration_updated(
    std::shared_ptr<DisplayConfiguration const> const& base_config)
{
    for_each_observer(&mg::DisplayConfigurationObserver::base_configuration_updated, base_config);
}

void mg::DisplayConfigurationObserverMultiplexer::session_configuration_applied(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<DisplayConfiguration> const& config)
{
    for_each_observer(&mg::DisplayConfigurationObserver::session_configuration_applied, session, config);
}

void mg::DisplayConfigurationObserverMultiplexer::session_configuration_removed(
    std::shared_ptr<ms::Session> const& session)
{
    for_each_observer(&mg::DisplayConfigurationObserver::session_configuration_removed, session);
}

void mg::DisplayConfigurationObserverMultiplexer::configuration_failed(
    std::shared_ptr<mg::DisplayConfiguration const> const& attempted,
    std::exception const& error)
{
    for_each_observer(&mg::DisplayConfigurationObserver::configuration_failed, attempted, error);
}

void mg::DisplayConfigurationObserverMultiplexer::catastrophic_configuration_error(
    std::shared_ptr<mg::DisplayConfiguration const> const& failed_fallback,
    std::exception const& error)
{
    for_each_observer(&mg::DisplayConfigurationObserver::catastrophic_configuration_error, failed_fallback, error);
}

mg::DisplayConfigurationObserverMultiplexer::DisplayConfigurationObserverMultiplexer(
    std::shared_ptr<Executor> const& default_executor)
    : ObserverMultiplexer(*default_executor),
      executor{default_executor}
{
}
