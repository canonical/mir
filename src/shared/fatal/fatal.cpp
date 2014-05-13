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

#include "mir/fatal.h"
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

void mir::abort(char const* reason, ...)
{
    va_list args;

    // Keep this as simple as possible, avoiding any object construction and
    // minimizing the potential for heap operations between the error location
    // and the abort().
    va_start(args, reason);
    fprintf(stderr, "Mir fatal error: ");
    vfprintf(stderr, reason, args);
    fprintf(stderr, "\n");
    va_end(args);

    std::abort();
}

void mir::_fail_assertion(char const* file, int line, char const* expr)
{
    mir::abort("Assertion failed at %s:%d: %s", file, line, expr);
}
