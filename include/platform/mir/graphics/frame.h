/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_FRAME_H_
#define MIR_GRAPHICS_FRAME_H_

#include <mir/time/posix_timestamp.h>
#include <cstdint>

namespace mir { namespace graphics {

/**
 * Frame is a unique identifier for a frame displayed on an output.
 *
 * This MSC/UST terminology is used because that's what the rest of the
 * industry calls it:
 *   GLX: https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 *   WGL: https://www.opengl.org/registry/specs/OML/wgl_sync_control.txt
 *   EGL: https://bugs.chromium.org/p/chromium/issues/attachmentText?aid=178027
 *   Mesa: "get_sync_values" functions
 */
struct Frame
{
    typedef mir::time::PosixTimestamp Timestamp;

    int64_t msc = 0;   /**< Media Stream Counter */
    Timestamp ust;     /**< Unadjusted System Time */
};

}} // namespace mir::graphics

#endif // MIR_GRAPHICS_FRAME_H_
