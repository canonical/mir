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
    // Input events use CLOCK_REALTIME, and so we must...
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    std::chrono::nanoseconds now = std::chrono::nanoseconds(ts.tv_sec * 1000000000LL + ts.tv_nsec);
    std::chrono::nanoseconds age = now - when;

    char str[64];
    snprintf(str, sizeof str, "%lld (%ld.%06ld ms ago)",
             static_cast<long long>(when.count()),
             static_cast<long>(age.count() / 1000000LL),
             static_cast<long>(age.count() % 1000000LL));

    return std::string(str);
}

