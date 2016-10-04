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

#include "reports.h"

#include "mir/default_server_configuration.h"
#include "logging/display_configuration_report.h"
#include "mir/observer_multiplexer.h"

mir::report::Reports::Reports(DefaultServerConfiguration& server)
    : display_configuration_report{std::make_shared<logging::DisplayConfigurationReport>(server.the_logger())},
      display_configuration_multiplexer{server.the_display_configuration_observer_registrar()}
{
    display_configuration_multiplexer->register_interest(display_configuration_report);
}
