/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/logging/input_timestamp.h"
#include <cstdio>

using namespace std::chrono;
std::string mir::logging::input_timestamp(nanoseconds when)
{
    // Input events use CLOCK_MONOTONIC, and so we must...
    auto const now = steady_clock::now().time_since_epoch();
    long long const when_ns = when.count();
    long long const now_ns = duration_cast<nanoseconds>(now).count();
    long long const age_ns = now_ns - when_ns;

    char str[64];
    snprintf(str, sizeof str, "%lld (%lld.%06lldms ago)",
             when_ns, age_ns / 1000000LL, age_ns % 1000000LL);

    return std::string(str);
}
