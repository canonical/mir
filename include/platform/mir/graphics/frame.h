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

namespace mir { namespace graphics {

inline uint64_t current_ust(clockid_t clock_id)
{
    struct timespec now;
    clock_gettime(clock_id, &now);
    return now.tv_sec*1000000ULL + now.tv_nsec/1000;
}

/**
 * Frame is a unique identifier for a frame displayed on an output.
 *
 * This "MSC/UST" terminology is used because that's what the rest of the
 * industry calls it:
 *   GLX: https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 *   EGL: https://bugs.chromium.org/p/chromium/issues/attachmentText?aid=178027
 *   WGL: https://www.opengl.org/registry/specs/OML/wgl_sync_control.txt
 *
 * The simplistic uint64 types are intentional as all these values originate
 * from the display hardware and must be passed unmodified from the server to
 * clients, and clients of clients (even clients of clients of clients like a
 * GLX app under Xmir under Unity8 under USC)!
 */
struct Frame
{
    uint64_t msc = 0;  /**< Media Stream Counter: Counter of the frame
                            displayed by the output (or as close to it as
                            can be estimated). */
    clockid_t clock_id = CLOCK_MONOTONIC;
                       /**< The system clock identifier that 'ust' is measured
                            by. Display drivers usually use CLOCK_MONOTONIC
                            but not always. */
    uint64_t ust = 0;  /**< Unadjusted System Time in microseconds of the frame
                            displayed by the output, relative to:
                              clock_gettime(frame.clock_id, &tp);
                            or
                              mir::graphics::current_ust(frame.clock_id)
                            It is crucial that all processes use the same
                            clock_id when comparing Frame information. */
    uint64_t min_ust_interval = 0;
                       /**< The shortest time possible to the next frame you
                            should expect. This value may change over time and
                            should not be assumed to remain constant,
                            especially as variable framerate displays become
                            more common. */

    int64_t age() const // might be negative in some cases (in the future)
    {
        return static_cast<int64_t>(current_ust(clock_id) - ust);
    }

    Frame predict_next() const
    {
        return Frame{msc+1, clock_id, ust+min_ust_interval, min_ust_interval};
    }
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_FRAME_H_
