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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *         Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/fatal.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>

#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

void mir::fatal_error_abort(char const* reason, ...)
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

void mir::fatal_error_except(char const* reason, ...)
{
    char buffer[1024];
    va_list args;

    va_start(args, reason);
    vsnprintf(buffer, sizeof buffer, reason, args);
    va_end(args);

    BOOST_THROW_EXCEPTION(std::runtime_error(buffer));
}

void (*mir::fatal_error)(char const* reason, ...){&mir::fatal_error_except};
