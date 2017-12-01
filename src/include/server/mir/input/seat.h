/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "mir/geometry/rectangles.h"
#include "mir_toolkit/event.h"

#include <memory>
#include <vector>

namespace mir
{
using EventUPtr = std::unique_ptr<MirEvent, void(*)(MirEvent*)>;
namespace input
{
class Device;
class OutputInfo;

class Seat
{
public:
    Seat()=default;
    virtual ~Seat() = default;
    virtual void add_device(Device const& device) = 0;
    virtual void remove_device(Device const& device) = 0;
    virtual void dispatch_event(std::shared_ptr<MirEvent> const& event) = 0;
    virtual EventUPtr create_device_state() = 0;

    virtual void set_key_state(Device const& dev, std::vector<uint32_t> const& scan_codes) = 0;
    virtual void set_pointer_state(Device const& dev, MirPointerButtons buttons) = 0;
    virtual void set_cursor_position(float cursor_x, float cursor_y) = 0;
    virtual void set_confinement_regions(geometry::Rectangles const& regions) = 0;
    virtual void reset_confinement_regions() = 0;

    virtual geometry::Rectangle bounding_rectangle() const = 0;
    virtual input::OutputInfo output_info(uint32_t output_id) const = 0;
private:
    Seat(Seat const&) = delete;
    Seat& operator=(Seat const&) = delete;
};

}
}

#endif
