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

/**
 * Frame is a unique identifier for a frame displayed on an output.
 */
struct Frame
{
    /*
     * Using bare int types because we must maintain these values accurately
     * from the driver code all the way to clients, and clients of clients ad
     * infinitum. int64_t is used because that's what drivers tend to provide
     * and we also need to support negative durations (sometimes things are in
     * the future).
     *   Also note that we use Timestamp instead of std::chrono::time_point as
     * we need to be able to switch clocks dynamically at runtime, depending on
     * which one any given graphics driver claims to use. They will always use
     * one of the kernel clocks...
     */
    struct Timestamp
    {
        clockid_t clock_id;
        int64_t nanoseconds;

        Timestamp()
            : clock_id{CLOCK_MONOTONIC}, nanoseconds{0}
        {
        }
        // Not sure why gcc-4.9 (vivid) demands this. TODO
        Timestamp(clockid_t clk, int64_t ns)
            : clock_id{clk}, nanoseconds{ns}
        {
        }
        Timestamp(clockid_t clk, struct timespec const& ts)
            : clock_id{clk}, nanoseconds{ts.tv_sec*1000000000LL + ts.tv_nsec}
        {
        }
        static Timestamp now(clockid_t clock_id)
        {
            struct timespec ts;
            clock_gettime(clock_id, &ts);
            return Timestamp(clock_id, ts);
        }
    };

    /*
     *
     * This "MSC/UST" terminology is used because that's what the rest of the
     * industry calls it:
     *   GLX: https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
     *   WGL: https://www.opengl.org/registry/specs/OML/wgl_sync_control.txt
     *   EGL: https://bugs.chromium.org/p/chromium/issues/attachmentText?aid=178027
     */
    int64_t msc = 0;   /**< Media Stream Counter */
    Timestamp ust;     /**< Unadjusted System Time */
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_FRAME_H_
