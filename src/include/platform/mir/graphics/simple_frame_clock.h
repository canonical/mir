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

#ifndef MIR_GRAPHICS_SIMPLE_FRAME_CLOCK_H_
#define MIR_GRAPHICS_SIMPLE_FRAME_CLOCK_H_

#include "frame_clock.h"
#include <mutex>

namespace mir
{
namespace graphics
{

class SimpleFrameClock : virtual public FrameClock
{
public:
    virtual ~SimpleFrameClock() = default;
    virtual void set_frame_callback(FrameCallback const&) override;
protected:
    typedef std::mutex FrameMutex;
    mutable FrameMutex frame_mutex;
    FrameCallback frame_callback;
};

}
}

#endif // MIR_GRAPHICS_SIMPLE_FRAME_CLOCK_H_
