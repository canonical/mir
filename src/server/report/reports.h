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

#ifndef MIR_REPORT_REPORTS_H_
#define MIR_REPORT_REPORTS_H_

#include <memory>

namespace mir
{
class DefaultServerConfiguration;

template<class Observer>
class ObserverRegistrar;

namespace graphics
{
class DisplayConfigurationObserver;
}
namespace input
{
class SeatObserver;
}
namespace options
{
class Option;
}

namespace frontend
{
class SessionMediatorObserver;
}

namespace report
{
namespace logging
{
class DisplayConfigurationReport;
}

class ReportFactory;

class Reports
{
public:
    Reports(DefaultServerConfiguration& server, options::Option const& options);

private:
    std::shared_ptr<logging::DisplayConfigurationReport> const display_configuration_report;
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const display_configuration_multiplexer;
    std::shared_ptr<input::SeatObserver> const seat_report;
    std::shared_ptr<ObserverRegistrar<input::SeatObserver>> const seat_observer_multiplexer;
    std::shared_ptr<frontend::SessionMediatorObserver> const session_mediator_report;
    std::shared_ptr<ObserverRegistrar<frontend::SessionMediatorObserver>> const
        session_mediator_observer_multiplexer;
};
}
}

#endif //MIR_REPORT_REPORTS_H_
