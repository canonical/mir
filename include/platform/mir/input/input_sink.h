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
    /**
     * Confine position of a pointer
     */
    virtual void confine_pointer(mir::geometry::Point& position) = 0;

    /**!
     * Obtain the bounding rectangle of the destination area for this input sink
     */
    virtual mir::geometry::Rectangle bounding_rectangle() const = 0;

private:
    InputSink(InputSink const&) = delete;
    InputSink& operator=(InputSink const&) = delete;
};
}
}

#endif
