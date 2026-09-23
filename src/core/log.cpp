/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
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
#define MIR_LOG_DEFAULT_TAGS { mir::logging::uncategorised() }
#include <mir/log.h>
#include <mir/logging/logger.h>
#include <chrono>
#include <exception>
#include <format>

#include <boost/exception/diagnostic_information.hpp>
#include <source_location>

namespace mir {

log<logging::Severity, logging::Tags, std::string_view, std::format_args>::log(
    logging::Severity sev,
    logging::Tags tags,
    std::string_view fmt,
    std::format_args args,
    std::source_location loc)
{
    logging::log(sev, tags, fmt, args, loc);
}

log<logging::Severity, logging::Tags, std::exception_ptr const&, std::string_view, std::format_args>::log(
    logging::Severity severity,
    logging::Tags tags,
    std::exception_ptr const& ex,
    std::string_view fmt,
    std::format_args args,
    std::source_location loc)
{
    try
    {
        std::rethrow_exception(ex);
    }
    catch(std::exception const& err)
    {
        // TODO: We can probably format this better by pulling out
        // the boost::errinfo's ourselves.
        mir::log<logging::Severity, logging::Tags, std::format_string<std::string const&, std::string const&>, std::string const&, std::string const&>(
            severity,
            tags,
            "{}: {}",
            std::vformat(fmt, args),
            boost::diagnostic_information(err),
            loc);
    }
    catch(...)
    {
        mir::log<logging::Severity, logging::Tags, std::format_string<std::string const&>, std::string const&>(
            severity,
            tags,
            "{}: unknown exception",
            std::vformat(fmt, args),
            loc);
    }
}

log<logging::Severity, logging::Tags, std::string_view>::log(
    logging::Severity severity,
    logging::Tags tags,
    std::string_view message,
    std::source_location loc)
{
    logging::log(severity, tags, message, std::make_format_args(), loc);
}

void security_log(
    logging::Severity severity,
    std::string const& event,
    std::string const& description)
{
    auto const now = std::chrono::system_clock::now();
    auto const datetime = std::format("{:%FT%TZ}", now);

    auto message = std::format(
        "{{"
            "\"datetime\": \"{}\", "
            "\"appid\": \"{}\", "
            "\"event\": \"{}\", "
            "\"level\": \"{}\", "
            "\"description\": \"{}\" "
        "}}",
        datetime,
        program_invocation_short_name ? program_invocation_short_name : "<unknown>",
        event,
        (severity == logging::Severity::critical ? "CRITICAL" :
         severity == logging::Severity::error ? "ERROR" :
         severity == logging::Severity::warning ? "WARN" :
         severity == logging::Severity::informational ? "INFO" :
         severity == logging::Severity::debug ? "DEBUG" : "UNKNOWN"),
        description
    );
    logging::log(severity, message, "security");
}

} // namespace mir
