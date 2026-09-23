
/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/logging/tag.h"
#include <format>
#include <mir/logging/shared_library_prober_report.h>
#include <mir/logging/logger.h>
#define MIR_LOG_DEFAULT_TAGS { mir::logging::uncategorised() }
#include <mir/log.h>

namespace ml = mir::logging;

ml::SharedLibraryProberReport::SharedLibraryProberReport(std::shared_ptr<Logger> const& logger)
    : logger{logger}
{
}

void ml::SharedLibraryProberReport::probing_path(std::filesystem::path const& path)
{
    auto const disp = path.string();
    logger->log(
        ml::Event{
            ml::Severity::informational,
            { ml::uncategorised() },
            "Loading modules from: {}",
            std::make_format_args(disp)
        });
}

void ml::SharedLibraryProberReport::probing_failed(std::filesystem::path const& path, std::exception const& error)
{
    auto const disp_path = path.string();
    auto const disp_err = error.what();
    logger->log(
        ml::Event{
            ml::Severity::error,
            { ml::uncategorised() },
            "Failed to load libraries from path: {} (error was: {})",
            std::make_format_args(disp_path, disp_err)
        });
}

void ml::SharedLibraryProberReport::loading_library(std::filesystem::path const& filename)
{
    auto const disp_filename = filename.string();
    logger->log(
        ml::Event{
            ml::Severity::informational,
            { ml::uncategorised() },
            "Loading module: {}",
            std::make_format_args(disp_filename)
        });
}

void ml::SharedLibraryProberReport::loading_failed(std::filesystem::path const& filename, std::exception const& error)
{
    auto const disp_filename = filename.string();
    auto const disp_error = error.what();
    logger->log(
        ml::Event{
            ml::Severity::warning,
            { ml::uncategorised() },
            "Failed to load module: {} (error was: {} ",
            std::make_format_args(disp_filename, disp_error)
        });
}
