/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_WAYLAND_TIMESPEC_H
#define MIR_FRONTEND_WAYLAND_TIMESPEC_H

#include <mir/time/types.h>

namespace mir
{
namespace frontend
{

struct WaylandTimespec
{
    WaylandTimespec(time::Timestamp time)
        : tv_sec_hi{static_cast<uint32_t>(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
              time.time_since_epoch()).count()) >> 32)},
          tv_sec_lo{static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(
              time.time_since_epoch()).count())},
          tv_nsec{static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
              time.time_since_epoch()).count() % 1000000000)}
    {
    }

    uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;
};

}
}

#endif // MIR_FRONTEND_WAYLAND_TIMESPEC_H
