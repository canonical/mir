/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_SINK_H_
#define MIR_INPUT_INPUT_SINK_H_

#include "mir_toolkit/event.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"

#include <vector>

namespace mir
{
namespace input
{
class InputSink
{
public:
    InputSink() = default;
    virtual ~InputSink() = default;
    virtual void handle_input(MirEvent& event) = 0;
    /**!
     * Obtain the bounding rectangle of the destination area for this input sink
     */
    virtual mir::geometry::Rectangle bounding_rectangle() const = 0;

    /**
     * \name Device State interface of InputSink
     *
     * In scenarios in which the device is not capable of seeing all changes as they occur,
     * these method should be used to update the input device state as needed
     * \{
     */
    /**
     * Set all pressed scan codes.
     * \param scan_codes currently pressed
     */
    virtual void set_key_state(std::vector<uint32_t> const& scan_codes) = 0;
    /**
     * Set button state of a pointing device.
     * \param buttons mask of the buttons currently pressed
     */
    virtual void set_pointer_state(MirPointerButtons buttons) = 0;
    /**
     * \}
     */
private:
    InputSink(InputSink const&) = delete;
    InputSink& operator=(InputSink const&) = delete;
};
}
}

#endif
