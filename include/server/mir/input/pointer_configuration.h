/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_POINTER_CONFIGURATION_H_
#define MIR_INPUT_POINTER_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mir_input_device.h"

namespace mir
{
namespace input
{

struct PointerConfiguration
{
    PointerConfiguration() {}

    PointerConfiguration(MirPointerHandedness handedness, bool enable_cursor_acceleration, double acceleration_bias,
                         double horizontal_scroll_scale, double vertical_scroll_scale)
        : handedness{handedness}, enable_cursor_acceleration{enable_cursor_acceleration},
          cursor_acceleration_bias{acceleration_bias}, horizontal_scroll_scale{horizontal_scroll_scale},
          vertical_scroll_scale{vertical_scroll_scale}
    {
    }

    /*!
     * Configure which button shall be used as primary button. That way the input device is configured to be either
     * right or left handed.
     */
    MirPointerHandedness handedness{mir_pointer_handedness_right};

    /*!
     * Configure whether mouse acceleration should be applied.
     */
    bool enable_cursor_acceleration{true};

    /*!
     * Configures the intensity of the cursor acceleration. Values within the range of [-1, 1] are allowed.
     *   - 0: default acceleration
     *   - [-1, 0): reduced acceleration
     *   - (0, 1]: increased acceleration
     */
    double cursor_acceleration_bias{0.0};

    /*!
     * Configures a signed scale of the horizontal scrolling. Use negative values to configure 'natural scrolling'
     */
    double horizontal_scroll_scale{1.0};

    /*!
     * Configures a signed scale of the vertical scrolling. Use negative values to configure 'natural scrolling'
     */
    double vertical_scroll_scale{1.0};
};

}
}

#endif
