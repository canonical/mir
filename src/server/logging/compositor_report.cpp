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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "compositor_report.h"
#include "mir/logging/logger.h"

using namespace mir;

namespace
{
    const char * const component = "compositor";
}

logging::CompositorReport::CompositorReport(
    std::shared_ptr<Logger> const& logger)
    : logger(logger)
{
}

logging::CompositorReport::TimePoint logging::CompositorReport::now()
{
    return std::chrono::steady_clock::now();
}

void logging::CompositorReport::begin_frame(Id id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    inst.start_of_frame = now();
}

void logging::CompositorReport::end_frame(Id id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto duration = now() - inst.start_of_frame;
    (void)duration; // TODO
    logger->log(Logger::informational, "TODO end_frame", component);
}
