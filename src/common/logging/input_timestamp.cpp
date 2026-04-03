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

#include <mir/logging/input_timestamp.h>
#include <cstdlib>
#include <format>

using namespace std::chrono;

std::string mir::logging::input_timestamp(mir::time::Clock const& clock, std::chrono::nanoseconds when)
{
    auto const now = clock.now().time_since_epoch();
    long long const when_ns = when.count();
    long long const now_ns = duration_cast<nanoseconds>(now).count();
    long long const age_ns = now_ns - when_ns;

    long long const abs_age_ns = std::llabs(age_ns);
    long long const milliseconds = abs_age_ns / 1000000LL;
    long long const sub_milliseconds = abs_age_ns % 1000000LL;

    char const* sign_suffix = (age_ns < 0) ? "in the future" : "ago";

    return std::format("{} ({}.{:06}ms {})",
        when_ns, milliseconds, sub_milliseconds, sign_suffix);
}
