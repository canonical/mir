/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TIME_CLOCK_H_
#define MIR_TIME_CLOCK_H_

#include "mir/time/types.h"

namespace mir
{
namespace time
{

class Clock
{
public:
    virtual ~Clock() = default;

    /**
     * The current time according to this clock.
     */
    virtual Timestamp now() const = 0;

    /**
     * The minimum amount of real time we would have to wait for this
     * clock to reach or surpass the specified timestamp.
     *
     * For clocks that deal in real time (i.e., most production
     * implementations), this will just be max(t - now(), 0).
     * However, fake clocks may return different durations.
     */
    virtual Duration min_wait_until(Timestamp t) const = 0;

protected:
    Clock() = default;
    Clock(Clock const&) = delete;
    Clock& operator=(Clock const&) = delete;
};

}
}

#endif // MIR_TIME_CLOCK_H_
