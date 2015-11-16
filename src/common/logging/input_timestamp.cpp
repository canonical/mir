/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/logging/input_timestamp.h"
#include <cstdio>

std::string mir::logging::input_timestamp(std::chrono::nanoseconds when)
{
    // Input events use CLOCK_MONOTONIC, and so we must...
    auto age =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch() - when);

    char str[64];
    snprintf(str, sizeof str, "%lld (%.6fms ago)",
             static_cast<long long>(when.count()),age.count());

    return std::string(str);
}
