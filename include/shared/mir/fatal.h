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
 * ---
 * Fatal error handling - Fatal errors are situations we don't expect to ever
 * happen and don't have logic to gracefully recover from. The most useful
 * thing you can do in that situation is abort to get a clean core file and
 * stack trace to maximize the chances of it being readable.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_FATAL_H_
#define MIR_FATAL_H_

namespace mir
{
/**
 * Kill the program and dump core as cleanly as possible. Note this is why the
 * parameter is a C-style string; to avoid any possible side-effects of
 * constructing new objects or touching the heap after an error has occurred.
 * mir::abort() is strictly for "this should never happen" situations that
 * you cannot recover from.
 *  \param [in] reason  A printf-style format string.
 */
void abort(char const* reason, ...);

/**
 * Mir-specific assert() function that is NEVER optimized out, always tested
 * even in release builds.
 */
#define mir_assert(expr) \
{ \
    if (!(expr)) ::mir::_fail_assertion(__FILE__, __LINE__, #expr); \
}
void _fail_assertion(char const* file, int line, char const* expr);

} // namespace mir

#endif // MIR_FATAL_H_
