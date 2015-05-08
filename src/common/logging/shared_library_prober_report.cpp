
/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/logging/shared_library_prober_report.h"
#include "mir/logging/logger.h"
#include "mir/log.h"

namespace ml = mir::logging;

ml::SharedLibraryProberReport::SharedLibraryProberReport(std::shared_ptr<Logger> const& logger)
    : logger{logger}
{
}

void ml::SharedLibraryProberReport::probing_path(boost::filesystem::path const& path)
{
    logger->log(ml::Severity::informational,
                std::string("Loading modules from: ") + path.string(),
                MIR_LOG_COMPONENT);
}

void ml::SharedLibraryProberReport::probing_failed(boost::filesystem::path const& path, std::exception const& error)
{
    logger->log(ml::Severity::error,
                std::string("Failed to load libraries from path: ") + path.string() +
                " (error was:" + error.what() + ")",
                MIR_LOG_COMPONENT);
}

void ml::SharedLibraryProberReport::loading_library(boost::filesystem::path const& filename)
{
    logger->log(ml::Severity::informational,
                std::string("Loading module: ") + filename.string(),
                MIR_LOG_COMPONENT);
}

void ml::SharedLibraryProberReport::loading_failed(boost::filesystem::path const& filename, std::exception const& error)
{
    logger->log(ml::Severity::warning,
                std::string("Failed to load module: ") + filename.string() +
                " (error was:" + error.what() + ")",
                MIR_LOG_COMPONENT);
}
