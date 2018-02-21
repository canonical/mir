/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_GRAPHICS_TRANSFORMATION_H_
#define MIR_GRAPHICS_TRANSFORMATION_H_

#include <mir_toolkit/common.h>
#include <glm/glm.hpp>

namespace mir { namespace graphics {

inline glm::mat2 transformation(MirOrientation ori)
{
    int cos = 0, sin = 0;
    switch (ori)
    {
    case mir_orientation_normal:   sin =  0; cos =  1; break;
    case mir_orientation_left:     sin =  1; cos =  0; break;
    case mir_orientation_inverted: sin =  0; cos = -1; break;
    case mir_orientation_right:    sin = -1; cos =  0; break;
    }
    return glm::mat2(cos, sin, -sin, cos);  // column-major order, GL-style
}

inline glm::mat2 transformation(MirMirrorMode mode)
{
    glm::mat2 mat{};
    if (mode == mir_mirror_mode_horizontal)
        mat[0][0] = -1;
    else if (mode == mir_mirror_mode_vertical)
        mat[1][1] = -1;
    return mat;
}

} } // namespace mir::graphics

#endif // MIR_GRAPHICS_TRANSFORMATION_H_
