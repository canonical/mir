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

#ifndef MIR_GRAPHICS_FRAME_H_
#define MIR_GRAPHICS_FRAME_H_

#include <cstdint>
#include <functional>

namespace mir
{
namespace graphics
{

/**
 * Unique identifier of a frame displayed to the user.
 *
 * This weird terminology is actually pretty standard in graphics texts,
 * OpenGL and Xorg. For more information see:
 *   https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 */
struct Frame
{
    uint64_t ust = 0;  /**< Unadjusted System Time in nanoseconds: This must
                            stay in its raw integer form so that (GLX) clients
                            can match their CLOCK_MONOTONIC to it. */
    uint64_t msc = 0;  /**< Media Stream Counter: The physical frame count
                            from the display hardware (or as close to it as
                            can be calculated).
                            You should not make the assumption that this
                            starts low, or that successive queries will
                            increase by one. Because it's entirely possible
                            that you missed a frame or frames so the delta
                            between msc values may be greater than one. */
};

class FrameClock
{
public:
    typedef std::function<void(Frame const&)> FrameCallback;

    virtual ~FrameClock() = default;
    virtual Frame last_frame() const = 0;
    /// For simplicity you should assume only one callback is supported
    virtual void on_next_frame(FrameCallback const&) = 0;
};

}
}

#endif // MIR_GRAPHICS_FRAME_H_
