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

namespace mir
{
namespace graphics
{

/**
 * Frame is a unique identifier for a frame displayed on a physical output.
 *
 * This weird terminology is actually very standard in graphics texts,
 * OpenGL and Xorg. The basic design is best described in GLX_OML_sync_control:
 *   https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 * which has also been copied by Google/Intel/Mesa into EGL as a simpler
 * informal extension: EGL_CHROMIUM_sync_control
 * What the GLX_OML_sync_control spec won't tell you is that UST is always
 * measured in microseconds relative to CLOCK_MONOTONIC.
 */
struct Frame
{
    uint64_t msc = 0;  /**< Media Stream Counter: Counter of the frame
                            displayed by the output (or as close to it as
                            can be estimated). */
    uint64_t ust = 0;  /**< Unadjusted System Time in microseconds (relative
                            to the kernel's CLOCK_MONOTONIC) of the frame
                            displayed by the output.
                            This value should remain unmodified through to
                            the client for accurate timing calculations. */

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
