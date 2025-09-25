/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_REPORT_LOGGING_REPORT_FACTORY_H_
#define MIR_REPORT_LOGGING_REPORT_FACTORY_H_

#include "report_factory.h"

namespace mir
{
namespace logging
{
class Logger;
}
namespace time
{
class Clock;
}
class DefaultServerConfiguration;
namespace report
{

class LoggingReportFactory : public report::ReportFactory
{
public:
    LoggingReportFactory(std::shared_ptr<mir::logging::Logger> const& logger,
                         std::shared_ptr<time::Clock> const& clock);
    std::shared_ptr<compositor::CompositorReport> create_compositor_report() override;
    std::shared_ptr<graphics::DisplayReport> create_display_report() override;
    std::shared_ptr<scene::SceneReport> create_scene_report() override;

    std::shared_ptr<input::InputReport> create_input_report() override;
    std::shared_ptr<input::SeatObserver> create_seat_report() override;
    std::shared_ptr<mir::SharedLibraryProberReport> create_shared_library_prober_report() override;
    std::shared_ptr<shell::ShellReport> create_shell_report() override;

private:
    std::shared_ptr<mir::logging::Logger> const logger;
    std::shared_ptr<time::Clock> const clock;
};
}
}

#endif
