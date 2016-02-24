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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TOOLKIT_MIR_DISPLAY_CONFIGURATION_H_
#define MIR_TOOLKIT_MIR_DISPLAY_CONFIGURATION_H_

#include "client_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Release resources associated with a MirDisplayConfig handle.
 *
 * \param [in]  config              The handle to release
 */
void mir_display_config_release(MirDisplayConfig* config);

int mir_display_config_get_max_simultaneous_outputs(MirDisplayConfig const* config);

int mir_display_config_get_num_outputs(MirDisplayConfig const* config);

MirOutput const* mir_display_config_get_output(MirDisplayConfig const* config, size_t index);

MirOutput* mir_display_config_get_mutable_output(MirDisplayConfig* config, size_t index);

int mir_output_get_num_modes(MirOutput const* output);

int mir_output_get_preferred_mode(MirOutput const* output);

int mir_output_get_current_mode(MirOutput const* output);

void mir_output_set_current_mode(MirOutput* output, int index);

MirDisplayMode const* mir_output_get_mode(MirOutput const* output, size_t index);

int mir_output_get_num_output_formats(MirOutput const* output);

MirPixelFormat mir_output_get_format(MirOutput const* output, size_t index);

MirPixelFormat mir_output_get_current_format(MirOutput const* output);

void mir_output_set_format(MirOutput* output, int index);

int mir_output_get_id(MirOutput const* output);

MirDisplayOutputType mir_output_get_type(MirOutput const* output);

int mir_output_get_position_x(MirOutput const* output);

int mir_output_get_position_y(MirOutput const* output);

bool mir_output_is_connected(MirOutput const* output);

bool mir_output_is_enabled(MirOutput const* output);

void mir_output_enable(MirOutput* output);

void mir_output_disable(MirOutput* output);

int mir_output_physical_width_mm(MirOutput const* output);

int mir_output_physical_height_mm(MirOutput const* output);

MirPowerMode mir_output_get_power_mode(MirOutput const* output);

void mir_output_set_power_mode(MirOutput* output, MirPowerMode mode);

MirOrientation mir_output_get_orientation(MirOutput const* output);

void mir_output_set_orientation(MirOutput* output, MirOrientation orientation);

#ifdef __cplusplus
}
#endif

#endif //MIR_TOOLKIT_MIR_DISPLAY_CONFIGURATION_H_
