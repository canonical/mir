/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TIME_HIGH_RESOLUTION_CLOCK_H_
#define MIR_TIME_HIGH_RESOLUTION_CLOCK_H_

#include "mir/time/clock.h"

namespace mir
{
namespace time
{

class HighResolutionClock : public Clock
{
public:
    virtual Timestamp sample() const override;

private:
    std::chrono::high_resolution_clock clock;
};
}
}

#endif /* MIR_TIME_HIGH_RESOLUTION_CLOCK_H_ */
