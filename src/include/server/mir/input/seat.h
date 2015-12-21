/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_SEAT_H_
#define MIR_INPUT_SEAT_H_

#include "mir/geometry/rectangle.h"
#include "mir_toolkit/event.h"

#include <memory>

namespace mir
{
namespace input
{
class Device;
class Seat
{
public:
    Seat()=default;
    virtual ~Seat() = default;
    virtual void add_device(std::shared_ptr<Device> const& device) = 0;
    virtual void remove_device(std::shared_ptr<Device> const& device) = 0;
    virtual void dispatch_event(MirEvent& event) = 0;
    virtual geometry::Rectangle get_rectangle_for(std::shared_ptr<Device> const& dev) = 0;
private:
    Seat(Seat const&) = delete;
    Seat& operator=(Seat const&) = delete;
};

}
}

#endif
