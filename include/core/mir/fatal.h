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
 *
 * ---
 * Fatal error handling - Fatal errors are situations we don't expect to ever
 * happen and don't have logic to gracefully recover from. The most useful
 * thing you can do in that situation is abort to get a clean core file and
 * stack trace to maximize the chances of it being readable.
 */

#ifndef MIR_FATAL_H_
#define MIR_FATAL_H_

#include <format>
#include <source_location>
#include <string_view>

namespace mir
{
/**
 * fatal_error() is strictly for "this should never happen" situations that you
 * cannot recover from. Kills the program and dump core as cleanly as possible.
 *   \param [in] reason  A printf-style format string.
 */
[[noreturn]]
void fatal_error(char const* reason, ...) __attribute__((format(printf, 1, 2)));

/**
 * fatal_error() is strictly for "this should never happen" situations that you
 * cannot recover from. Kills the program and dump core as cleanly as possible.
 *   \param [in] loc  The source location where the fatal error occurred.
 *   \param [in] message  A message describing the fatal error.
 */
[[noreturn]]
void fatal_error(std::source_location const loc, std::string_view message);

template<typename... Args>
[[noreturn]] inline void fatal_error(std::source_location const loc, std::format_string<Args...> reason, Args&&... args)
{ fatal_error(loc, std::format(reason, std::forward<Args>(args)...)); }
} // namespace mir

#define MIR_FATAL_ERROR(...) ::mir::fatal_error(::std::source_location::current(), __VA_ARGS__)

#endif // MIR_FATAL_H_
