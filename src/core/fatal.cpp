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

#include <mir/fatal.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

[[noreturn]]
void mir::fatal_error_abort(char const* reason, ...)
{
    va_list args;

    // Keep this as simple as possible, avoiding any object construction and
    // minimizing the potential for heap operations between the error location
    // and the abort().
    va_start(args, reason);
    std::fprintf(stderr, "Mir fatal error: ");
    std::vfprintf(stderr, reason, args);
    std::fprintf(stderr, "\n");
    va_end(args);

    std::abort();
}

void (*mir::fatal_error)(char const* reason, ...){&mir::fatal_error_abort};
