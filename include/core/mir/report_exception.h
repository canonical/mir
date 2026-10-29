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

#ifndef MIR_REPORT_EXCEPTION_H_
#define MIR_REPORT_EXCEPTION_H_

#include <exception>
#include <format>
#include <iosfwd>
#include <string>

namespace mir
{
/**
 *  Call from a catch block (and only from a catch block) to write error information to the appropriate stream.
 */
void report_exception(std::ostream& out_stream, std::ostream& err_stream);

/**
 *  Call from a catch block (and only from a catch block) to write error information to the given stream.
 */
void report_exception(std::ostream& stream);

/**
 *  Call from a catch block (and only from a catch block) to write error information to stdout or stderr as appropriate.
 */
void report_exception();

/**
 *  Wraps an exception for std::format, e.g. std::format("{}", mir::ExceptionDiagnostics{e}).
 *  Formats as boost::diagnostic_information(): what() together with any boost annotations
 *  (such as the throw location added by BOOST_THROW_EXCEPTION). The result can span
 *  several lines.
 */
struct ExceptionDiagnostics
{
    std::exception const& error;
};
}

/**
 *  Format specs for strings (fill, alignment, width) are supported.
 */
template<>
struct std::formatter<mir::ExceptionDiagnostics> : std::formatter<std::string>
{
    auto format(mir::ExceptionDiagnostics const& diagnostics, std::format_context& ctx) const
        -> std::format_context::iterator;
};

#endif /* MIR_REPORT_EXCEPTION_H_ */
