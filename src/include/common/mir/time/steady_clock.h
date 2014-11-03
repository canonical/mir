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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TIME_STEADY_CLOCK_H_
#define MIR_TIME_STEADY_CLOCK_H_

#include "mir/time/clock.h"

namespace mir
{
namespace time
{

class SteadyClock : public Clock
{
public:
    Timestamp now() const override;
    Duration min_wait_until(Timestamp t) const override;

private:
    std::chrono::steady_clock clock;
};
}
}

#endif
