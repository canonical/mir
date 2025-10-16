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

#ifndef MIR_INPUT_TOUCHSCREEN_SETTINGS_H
#define MIR_INPUT_TOUCHSCREEN_SETTINGS_H

#include <mir_toolkit/client_types.h>
#include <mir_toolkit/mir_input_device_types.h>

namespace mir
{
namespace input
{

struct TouchscreenSettings
{
    TouchscreenSettings() {}
    TouchscreenSettings(uint32_t output_id, MirTouchscreenMappingMode mode)
        : output_id{output_id}, mapping_mode{mode}
    {}

    /**
     * Configures the output the input device should map its coordinates to.
     * The value of this setting is only relevant when the mapping mode is
     * set up to map to a single output.
     */
    uint32_t output_id{0};
    /**
     * Selects the mapping mode.
     */
    MirTouchscreenMappingMode mapping_mode{mir_touchscreen_mapping_mode_to_display_wall};
};

inline bool operator==(TouchscreenSettings const& lhs, TouchscreenSettings const& rhs)
{
    return lhs.output_id == rhs.output_id && lhs.mapping_mode == rhs.mapping_mode;
}
}
}

#endif
