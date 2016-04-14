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

#ifndef MIR_GRAPHICS_FRAME_CLOCK_H_
#define MIR_GRAPHICS_FRAME_CLOCK_H_

#include "frame.h"
#include <functional>

namespace mir
{
namespace graphics
{

typedef std::function<void(Frame const&)> FrameCallback;

/**
 * A FrameClock is a source of 'Frame' counters to sync rendering to.
 */
class FrameClock
{
public:
    virtual ~FrameClock() = default;
    virtual void set_frame_callback(FrameCallback const&) = 0;
};

}
}

#endif // MIR_GRAPHICS_FRAME_CLOCK_H_
