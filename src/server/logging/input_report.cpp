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

#include "mir/logging/input_report.h"
#include "mir/logging/logger.h"

#include "std/MirLog.h"
#include <std/Log.h>

#include <mutex>

namespace ml = mir::logging;

namespace
{
std::mutex mutex;
std::shared_ptr<ml::InputReport> the_input_report;

class MyInputReport : public ml::InputReport
{
public:
    MyInputReport(std::shared_ptr<ml::Logger> const& logger) :
        logger(logger)
    {
    }

    void log(int prio, char const* buffer)
    {
        switch (prio)
        {
        case ANDROID_LOG_UNKNOWN:
        case ANDROID_LOG_DEFAULT:
        case ANDROID_LOG_VERBOSE:
        case ANDROID_LOG_DEBUG:
            logger->log<ml::Logger::debug>(buffer);
            break;

        case ANDROID_LOG_INFO:
            logger->log<ml::Logger::informational>(buffer);
            break;

        case ANDROID_LOG_WARN:
            logger->log<ml::Logger::warning>(buffer);
            break;

        case ANDROID_LOG_ERROR:
            logger->log<ml::Logger::error>(buffer);
        };
    }

private:
    std::shared_ptr<ml::Logger> const logger;
};

void my_write_to_log(int prio, char const* buffer)
{
    std::unique_lock<std::mutex> lock(mutex);
    the_input_report->log(prio, buffer);
}
}


std::shared_ptr<ml::InputReport>
ml::InputReport::initialize_input_report(std::shared_ptr<Logger> const& logger)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!::the_input_report)
        ::the_input_report = std::make_shared<MyInputReport>(logger);

    mir::write_to_log = my_write_to_log;

    return ::the_input_report;
}
