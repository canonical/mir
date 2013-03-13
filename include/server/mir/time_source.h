/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TIME_SOURCE_H_
#define MIR_TIME_SOURCE_H_

#include "mir/chrono/chrono.h"

#include <cstdint>

namespace mir
{

typedef std::chrono::high_resolution_clock::time_point Timestamp;

class TimeSource
{
public:
    virtual ~TimeSource() {}

    virtual Timestamp sample() const = 0;

protected:
    TimeSource() = default;
};

}

#endif // MIR_TIME_SOURCE_H_
