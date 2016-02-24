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

typedef struct MirOutputMode MirOutputMode;

/**
 * Release resources associated with a MirDisplayConfig handle.
 *
 * \param [in]  config              The handle to release
 */
void mir_display_config_release(MirDisplayConfig* config);

/**
 * Get the maximum possible number of simultaneously active outputs this system supports.
 *
 * \note There may be restrictions on the configuration required to achieve this many active
 *        outputs. Typically the achievable number of simultaneously active outputs is lower than
 *        this number.
 * \param [in]  config    The configuration to query
 * \returns      The maximum number of simultaneously active outputs supportable at this time.
 */
int mir_display_config_get_max_simultaneous_outputs(MirDisplayConfig const* config);

/**
 * Get the number of outputs in this display configuration.
 *
 * \param [in]  config    The configuration to query
 * \returns      The number of outputs available in this configuration.
 */
int mir_display_config_get_num_outputs(MirDisplayConfig const* config);

/**
 * Get a read-only handle to the \param index 'th output of this configuration
 *
 * \note The MirOutput handle is only valid while \param config is valid.
 * \pre 0 <= index < mir_display_config_get_num_outputs(config)
 * \param [in]  config  The configuration to query
 * \param [in]  index   The index of the output to get.
 * \returns     A read-only handle to a MirOutput within \param config which is valid until
 *              mir_display_config_release(config) is called.
 */
MirOutput const* mir_display_config_get_output(MirDisplayConfig const* config, size_t index);

/**
 * Get a handle to the \param index 'th output of this configuration which can be used
 * to change the state.
 *
 * \note The MirOutput handle is only valid while \param config is valid.
 * \pre 0 <= index < mir_display_config_get_num_outputs(config)
 * \param [in]  config  The configuration to query
 * \param [in]  index   The index of the output to get.
 * \returns     A handle to a MirOutput within \param config which is valid until
 *              mir_display_config_release(config) is called.
 */
MirOutput* mir_display_config_get_mutable_output(MirDisplayConfig* config, size_t index);

/**
 * Get the number of modes in the supported mode list of this output.
 *
 * The list of supported modes is retrieved from the hardware, possibly modified by any applicable quirk tables,
 * and may not be exhaustive.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The number of modes in the supported mode list of \param output.
 */
int mir_output_get_num_modes(MirOutput const* output);

/**
 * Get a handle for a mode descriptor from the list of supported modes.
 *
 * The list of supported modes is retrieved from the hardware, possibly modified by any applicable quirk tables,
 * and may not be exhaustive.
 *
 * \pre 0 ≤ index < mir_output_get_num_modes(output)
 * \note    The handle remains valid as long as \param output is valid.
 * \param [in]  output  The MirOutput to query
 * \param [in]  index   The index of the mode to retrieve.
 * \returns     A handle for a description of the supported mode. This is valid for as long as \param output is
 *              valid. The return value is never null.
 */
MirOutputMode const* mir_output_get_mode(MirOutput const* output, size_t index);

/**
 * Get a handle to the output's preferred mode.
 *
 * This is provided by the output itself. For modern displays (LCD, OLED, etc) it is typically a mode with the
 * native resolution.
 *
 * An output may not have a preferred mode, in which case this call will return NULL.
 *
 * \note    The handle remains valid as long as \param output is valid.
 * \param [in]  output  The MirOutput to query
 * \returns     A handle for a description of the supported mode. This is valid for as long as \param output is
 *              valid. If the output does not have a preferred mode, it returns NULL.
 */
MirOutputMode const* mir_output_get_preferred_mode(MirOutput const* output);

/**
 * Get a handle to the output's current mode.
 *
 * An output may not have a current mode (for example, if it is disabled),
 * in which case this call will return NULL.
 *
 * \note    The handle remains valid as long as \param output is valid.
 * \param [in]  output  The MirOutput to query
 * \returns     A handle for a description of the supported mode. This is valid for as long as \param output is
 *              valid. If the output does not have a current mode, it returns NULL.
 */
MirOutputMode const* mir_output_get_current_mode(MirOutput const* output);

/**
 * Set the current mode of an output.
 *
 * \note    Currently, \param mode must be a pointer returned from mir_output_get_mode.
 *          This limitation should be relaxed in future, allowing modes not on the supported mode list to be
 *          set.
 * \param [in]  output  The MirOutput to change
 * \param [in]  mode    Handle to the mode to set
 */
void mir_output_set_current_mode(MirOutput* output, MirOutputMode const* mode);

/**
 * Get the number of pixel formats supported by this output
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The number of pixel formats for \param output.
 */
int mir_output_get_num_output_formats(MirOutput const* output);

/**
 * Get a pixel format from the list of supported formats
 *
 * \pre 0 ≤ index < mir_output_get_num_output_formats(output)
 * \param [in]  output  The MirOutput to query
 * \param [in]  index   The index of the format to query
 * \returns     The \param index 'th supported pixel format.
 */
MirPixelFormat mir_output_get_format(MirOutput const* output, size_t index);

/**
 * Get the current pixel format.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The current pixel format. This may be mir_pixel_format_invalid (for example, if the output is not
 *              currently enabled).
 */
MirPixelFormat mir_output_get_current_format(MirOutput const* output);

/**
 * Set the output format
 *
 * \param [in]  output  The MirOutput to modify
 * \param [in]  format  The MirPixelFormat to set
 */
void mir_output_set_format(MirOutput* output, MirPixelFormat format);

/**
 * Get the ID of an output
 *
 * This can be used to refer to the output in other APIs, such as mir_surface_spec_set_fullscreen_on_output().
 *
 * \note    This may be removed in future, in favour of passing the MirOutput* directly to such APIs.
 * \param [in]  output  The MirOutput to query.
 * \returns     The ID of \param output, which may be used to refer to it in other APIs.
 */
int mir_output_get_id(MirOutput const* output);

/**
 * Get the physical connection type of an output.
 *
 * This is a best-effort determination, and may be incorrect.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The type of the display connection, or mir_display_output_type_unknown if it cannot be determined.
 */
MirDisplayOutputType mir_output_get_type(MirOutput const* output);

/**
 * Get the x coordinate of the top-left point of the output in the virtual display space.
 *
 * Outputs can be thought of as viewports into a virtual display space. They may freely overlap, coincide, or
 * be disjoint as desired.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The x coordinate of the top-left point of the output in the virtual display space.
 */
int mir_output_get_position_x(MirOutput const* output);

/**
 * Get the y coordinate of the top-left point of the output in the virtual display space.
 *
 * Outputs can be thought of as viewports into a virtual display space. They may freely overlap, coincide, or
 * be disjoint as desired.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The y coordinate of the top-left point of the output in the virtual display space.
 */
int mir_output_get_position_y(MirOutput const* output);

typedef enum
{
    mir_output_disconnected = 0,
    mir_output_connected,
    mir_output_connection_unknown
} MirOutputConnection;

/**
 * Get whether there is a display physically connected to the output.
 *
 * This gives a best-effort determination of whether or not enabling this output will result in
 * an image being displayed to the user.
 *
 * The accuracy of this determination varies with connection type - for example, for
 * DisplayPort and HDMI connections a return value of mir_output_connected is usually
 * a reliable indicator that there is a powered-on display connected.
 *
 * VGA and DVI connectors can usually determine whether or not there is a physically connected
 * display, but cannot distinguish between a powered or unpowered display.
 *
 * It is not always possible to determine whether or not there is a display connected;
 * in such cases mir_output_connection_unknown is returned.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     Whether there is a display connected to this output.
 */
MirOutputConnection mir_output_get_connection_state(MirOutput const *output);

/**
 * Get whether this output is enabled in the current configuration.
 *
 * \param [in]  output  the MirOutput to query
 * \returns     Whether the output is enabled.
 */
bool mir_output_is_enabled(MirOutput const* output);

/**
 * Enable the output.
 *
 * \param [in]  output  the MirOutput to enable
 */
void mir_output_enable(MirOutput* output);

/**
 * Disable the output.
 *
 * \param [in]  output  the MirOutput to disable
 */
void mir_output_disable(MirOutput* output);

/**
 * Get the physical width of the connected display, in millimetres.
 *
 * A best-effort report of the physical width of the display connected to
 * this output. This is retrieved from the display hardware, possibly modified by any applicable
 * quirk tables.
 *
 * Where this information is unavailable or inapplicable (for example, projectors),
 * 0 is returned.
 *
 * \param [in]  output  the MirOutput to query
 * \returns     Physical width of the connected display, in mm.
 */
int mir_output_get_physical_width_mm(MirOutput const *output);

/**
 * Get the physical height of the connected display, in millimetres.
 *
 * A best-effort report of the physical height of the display connected to
 * this output. This is retrieved from the display hardware, possibly modified by any applicable
 * quirk tables.
 *
 * Where this information is unavailable or inapplicable (for example, projectors),
 * 0 is returned.
 *
 * \param [in]  output  the MirOutput to query
 * \returns     Physical height of the connected display, in mm.
 */
int mir_output_get_physical_height_mm(MirOutput const *output);

/**
 * Get the power state of a connected display.
 *
 * It is undefined which power state is returned for an output which is not connected.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The power state of the display connected to \param output.
 */
MirPowerMode mir_output_get_power_mode(MirOutput const* output);

/**
 * Set the power state of a display.
 *
 * It is not an error to set the power state of an unconnected display, but it may have no effect.
 *
 * \param [in]  output  The MirOutput to modify
 * \param [in]  mode    The requested power state of \param output
 */
void mir_output_set_power_mode(MirOutput* output, MirPowerMode mode);

/**
 * Get the orientation of a display.
 *
 * \param [in]  output  The MirOutput to query
 * \returns     The orientation of \param output
 */
MirOrientation mir_output_get_orientation(MirOutput const* output);

/**
 * Set the orientation of a display.
 *
 * \param [in]  output  The MirOutput to modify
 * \param [in]  orientation The requested display orientation
 */
void mir_output_set_orientation(MirOutput* output, MirOrientation orientation);

/**
 * Get the resolution of a MirOutputMode
 *
 * \param [in]  mode    The MirOutputMode to query
 * \returns     A reference to a MirExtent containing the pixel width and pixel height of the mode.
 *              This is guaranteed non-NULL.
 */
MirExtent const* mir_output_mode_get_resolution(MirOutputMode const* mode);

/**
 * Get the refresh rate, in Hz, of a MirOutputMode
 *
 * \param [in]  mode    The MirOutputMode to query
 * \returns     The refresh rate, in Hz, of \param mode
 */
double mir_output_mode_get_refresh_rate(MirOutputMode const* mode);

#ifdef __cplusplus
}
#endif

#endif //MIR_TOOLKIT_MIR_DISPLAY_CONFIGURATION_H_
