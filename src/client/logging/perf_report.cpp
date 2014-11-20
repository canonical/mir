/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "perf_report.h"
#include "mir/logging/logger.h"
#include "mir/time/steady_clock.h"

using namespace mir::client;
namespace ml = mir::logging;

namespace
{
const char * const component = "perf"; // Note context is already within client
} // namespace

logging::PerfReport::PerfReport(
        std::shared_ptr<mir::logging::Logger> const& logger)
    : PeriodicPerfReport(std::chrono::seconds(1),
                         std::make_shared<mir::time::SteadyClock>())
    , logger(logger)
{
}

void logging::PerfReport::display(const char *name, long fps100,
                                  long rendertime_usec, long lag_usec,
                                  int nbuffers) const
{
    char msg[256];
    snprintf(msg, sizeof msg,
             "%s: %2ld.%02ld FPS, render time %ld.%02ldms, buffer lag %ld.%02ldms (%d buffers)",
             name,
             fps100 / 100, fps100 % 100,
             rendertime_usec / 1000, (rendertime_usec / 10) % 100,
             lag_usec / 1000, (lag_usec / 10) % 100,
             nbuffers
             );

    logger->log(ml::Severity::informational, msg, component);
}
