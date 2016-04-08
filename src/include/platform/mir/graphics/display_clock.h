/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_GRAPHICS_DISPLAY_CLOCK_H_
#define MIR_GRAPHICS_DISPLAY_CLOCK_H_

#include "frame.h"
#include <functional>

namespace mir
{
namespace graphics
{

typedef std::function<void(Frame const&)> FrameCallback;

/**
 * An abstract frame clock, which may represent a single phyisical display
 * or a group of displays.
 */
class DisplayClock
{
public:
    virtual ~DisplayClock() = default;
    /**
     * Last frame in which the display was actually used by the DisplayClock
     * owner, which may be older than the last frame on the physical display.
     */
    virtual Frame last_frame() const = 0;

    /**
     * Make a callback next time a frame is displayed by us, which for a
     * compositor may not be the next physical frame if the scene is idle.
     */
    virtual void on_next_frame(FrameCallback const&) = 0;
};

}
}

#endif // MIR_GRAPHICS_DISPLAY_CLOCK_H_
