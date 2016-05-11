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
#include <ctime>

namespace mir
{
namespace graphics
{

/**
 * Frame is a unique identifier for a frame displayed on a physical output.
 *
 * This weird terminology is actually very standard in graphics texts,
 * OpenGL and Xorg. The basic design is best described in:
 *   https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 *
 * The simplistic structure is intentional, as all these values need to
 * be passed from the server to clients and clients of clients unmodified.
 * Even clients of clients of clients (GLX app under Xmir under Unity8 under
 * USC).
 */
struct Frame
{
    uint64_t msc = 0;  /**< Media Stream Counter: Counter of the frame
                            displayed by the output (or as close to it as
                            can be estimated). */
    clockid_t clock_id = CLOCK_MONOTONIC;
                       /**< The system clock identifier that 'ust' is measured
                            by. Usually monotonic, but might not always be.
                            To get the current time relative to ust just
                            call: clock_gettime(clock_id, ...) */
    uint64_t ust = 0;  /**< Unadjusted System Time in microseconds of the frame
                            displayed by the output, relative to clock_id.
                            NOTE that comparing 'ust' to the current system
                            time (at least in the server process) you will
                            often find that 'ust' is in the future by a few
                            microseconds and not yet in the past. This is to be
                            expected and simply represents the reality that
                            scanning out a new frame takes longer than
                            returning from the flip or swap function. */

    bool operator<(Frame const& rhs) const
    {
        // Wrap-around would take 9.7 billion years on a 60Hz display, so
        // that's unlikely but handle it anyway: A>B iff 0 < A-B < (4.8B years)
        return (int64_t)(rhs.msc - msc) > 0;
    }
};

}
}

#endif // MIR_GRAPHICS_FRAME_H_
