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

/*
 * This weird terminology is actually pretty standard in graphics texts,
 * OpenGL and Xorg. For more information see:
 *   https://www.opengl.org/registry/specs/OML/glx_sync_control.txt
 */
struct Frame
{
    uint64_t ust = 0;  // Unadjusted System Time
    uint64_t msc = 0;  // Media Stream Counter
};

}
}

#endif // MIR_GRAPHICS_FRAME_H_
