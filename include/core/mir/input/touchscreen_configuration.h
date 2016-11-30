/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_INPUT_TOUCH_SCREEN_CONFIGURATION_H_
#define MIR_INPUT_TOUCH_SCREEN_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device_types.h"

namespace mir
{
namespace input
{

struct TouchscreenConfiguration
{
    TouchscreenConfiguration() {}

    TouchscreenConfiguration(uint32_t output_id, MirTouchscreenMappingMode mode)
       : output_id{output_id}, mapping_mode{mode}
    {}

    /**
     * Configures the output the device coordinates should be aligned to.
     *
     * This element is only relevant when mapping_mode is set to
     * mir_touchscreen_mapping_mode_to_output.
     */
    uint32_t output_id{0};

    /**
     * Configure the type of coordinate mapping to be used for this input
     * device.
     */
    MirTouchscreenMappingMode mapping_mode{mir_touchscreen_mapping_mode_to_output};
};

}
}

#endif
