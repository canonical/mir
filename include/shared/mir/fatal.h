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
 * Fatal error handling - Errors for which no run-time recovery exists and the
 * most useful thing you can do is dump core with a clean stack trace.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_FATAL_H_
#define MIR_FATAL_H_

namespace mir
{
/**
 * Kill the program and dump core as cleanly as possible. Note this is why the
 * parameter is a C-style string; to avoid an possible side-effects of
 * constructing new objects or touching the heap after an error has occurred.
 *  \param [in] reason  A printf-style format string.
 */
void abort(char const* reason, ...);

/**
 * Mir-specific assert() function that is NEVER optimized out, always tested
 * even in release builds.
 */
#define mir_assert(expr) { if (!(expr)) ::mir::abort(__FILE__ ":" __LINE__ \
                                                     " Assertion failed: " \
                                                     #expr); }

} // namespace mir

#endif // MIR_FATAL_H_
