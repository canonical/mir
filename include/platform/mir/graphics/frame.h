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

/*
 * Using bare types because it's critical we maintain these values accurately
 * from the driver code and then passed all the way to clients, and clients of
 * clients. Existing drivers and APIs seem to use int64_t for this, and it
 * has to be signed because sometimes time is negative (in the future).
 */
typedef int64_t Microseconds;

/*
 * Also note that we use Timestamp instead of std::chrono::time_point because
 * we need to be able to switch clocks dynamically at runtime, depending on
 * which one any given driver claims to use.
 */
struct Timestamp
{
    clockid_t clock_id;
    Microseconds microseconds;

    Timestamp() : clock_id{CLOCK_MONOTONIC}, microseconds{0} {}
    // Note sure why gcc-4.9 (vivid) demands this over {c,u}
    Timestamp(clockid_t c, Microseconds u) : clock_id{c}, microseconds{u} {}

    static Timestamp now(clockid_t clock_id)
    {
        struct timespec ts;
        clock_gettime(clock_id, &ts);
        return Timestamp(clock_id, ts.tv_sec*1000000LL + ts.tv_nsec/1000);
    }

    Microseconds age() const
    {
        return now(clock_id).microseconds - microseconds;
    }
};

/**
 * Frame is a unique identifier for a frame displayed on an output.
 *
 * This "MSC/UST" terminology is used because that's what the rest of the
 * industry calls it:
 *   GLX: https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 *   EGL: https://bugs.chromium.org/p/chromium/issues/attachmentText?aid=178027
 *   WGL: https://www.opengl.org/registry/specs/OML/wgl_sync_control.txt
 */
struct Frame
{
    int64_t msc = 0;   /**< Media Stream Counter */
    Timestamp ust;     /**< Unadjusted System Time */
    Microseconds min_ust_interval = 0;
                       /**< The minimum number of microseconds to the next
                            frame after this one. This value may change over
                            time and should not be assumed to remain constant,
                            especially as variable framerate displays become
                            more common. */

    Microseconds age() const // might be negative in some cases (in the future).
    {
        return ust.age();
    }

/* Unused?
    Frame predict_next() const
    {
        return Frame{msc+1,
                     {ust.clock_id, ust.microseconds+min_ust_interval},
                     min_ust_interval};
    }
*/
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_FRAME_H_
