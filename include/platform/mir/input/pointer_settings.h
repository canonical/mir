/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_POINTER_SETTINGS_H_
#define MIR_INPUT_POINTER_SETTINGS_H_

#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mir_input_device.h"

namespace mir
{
namespace input
{

struct PointerSettings
{
    PointerSettings() {}
    /**
     * Configure left and right handed mode by selecting a primary button
     */
    MirPointerHandedness handedness{mir_pointer_handedness_right};
    /**
     * Bias cursor acceleration.
     *   - [-1, 0): reduced acceleration
     *   - 0: default acceleration
     *   - (0, 1]: increased acceleration
     */
    double cursor_acceleration_bias{0.0};
    /**
     * Acceleration profile
     */
    MirPointerAcceleration acceleration{mir_pointer_acceleration_adaptive};
    /**
     * Scale horizontal scrolling linearly
     */
    double horizontal_scroll_scale{1.0};
    /**
     * Scale vertical scrolling linearly
     */
    double vertical_scroll_scale{1.0};
};

}
}

#endif
