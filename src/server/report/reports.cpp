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
#include "mir/options/option.h"
#include "logging/display_configuration_report.h"
#include "mir/observer_multiplexer.h"
#include "mir/options/configuration.h"
#include "mir/abnormal_exit.h"

#include "report_factory.h"
#include "lttng_report_factory.h"
#include "logging_report_factory.h"
#include "null_report_factory.h"

#include <string>

namespace mo = mir::options;
namespace mr = mir::report;

namespace
{
std::shared_ptr<mir::input::SeatObserver> create_seat_reports(
    mir::DefaultServerConfiguration& config,
    std::string const& opt)
{
    if (opt == mo::log_opt_value)
    {
        return mr::LoggingReportFactory(config.the_logger(), config.the_clock()).create_seat_report();
    }
    else if (opt == mo::lttng_opt_value)
    {
        return mr::LttngReportFactory{}.create_seat_report();
    }
    else if (opt == mo::off_opt_value)
    {
        return mr::NullReportFactory{}.create_seat_report();
    }
    else
    {
        throw mir::AbnormalExit(
            std::string("Invalid ") + mo::seat_report_opt + " option: " + opt + " (valid options are: \"" +
            mo::off_opt_value + "\" and \"" + mo::log_opt_value +
            "\" and \"" + mo::lttng_opt_value + "\")");
    }
}
}

mir::report::Reports::Reports(
    DefaultServerConfiguration& server,
    options::Option const& options)
    : display_configuration_report{std::make_shared<logging::DisplayConfigurationReport>(server.the_logger())},
      display_configuration_multiplexer{server.the_display_configuration_observer_registrar()},
      seat_report{create_seat_reports(server, options.get<std::string>(mo::seat_report_opt))},
      seat_observer_multiplexer{server.the_seat_observer_registrar()}
{
    display_configuration_multiplexer->register_interest(display_configuration_report);
    seat_observer_multiplexer->register_interest(seat_report);
}
