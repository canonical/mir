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

#ifndef MIR_LOGGING_INPUT_TIMESTAMP_H_
#define MIR_LOGGING_INPUT_TIMESTAMP_H_

#include <string>
#include <chrono>

namespace mir
{
namespace logging
{

template<typename Clock>
std::string input_timestamp(Clock const& clock, std::chrono::nanoseconds when)
{
    auto const now = clock.now().time_since_epoch();
    long long const when_ns = when.count();
    long long const now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    long long const age_ns = now_ns - when_ns;

    long long const abs_age_ns = std::llabs(age_ns);
    long long const milliseconds = abs_age_ns / 1000000LL;
    long long const sub_milliseconds = abs_age_ns % 1000000LL;

    char const* sign_suffix = (age_ns < 0) ? "in the future" : "ago";

    char str[64];
    snprintf(str, sizeof str, "%lld (%lld.%06lldms %s)",
             when_ns, milliseconds, sub_milliseconds, sign_suffix);

    return std::string(str);
}

}
}

#endif // MIR_LOGGING_INPUT_TIMESTAMP_H_
