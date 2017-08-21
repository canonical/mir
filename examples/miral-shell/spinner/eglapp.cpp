/*
 * Copyright © 2013, 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eglapp.h"

#include <mir/client/display_config.h>

#include "miregl.h"


float mir_eglapp_background_opacity = 1.0f;


namespace
{
MirPixelFormat select_pixel_format(MirConnection* connection)
{
    unsigned int format[mir_pixel_formats];
    unsigned int nformats;

    mir_connection_get_available_surface_formats(
        connection,
        (MirPixelFormat*) format,
        mir_pixel_formats,
        &nformats);

    auto const pixel_format = (MirPixelFormat) format[0];

    printf("Server supports %d of %d surface pixel formats. Using format: %d\n",
           nformats, mir_pixel_formats, pixel_format);

    return pixel_format;
}
}

std::vector<std::shared_ptr<MirEglSurface>> mir_eglapp_init(MirConnection* const connection)
{
    char const * const name = "eglappsurface";

    if (!mir_connection_is_valid(connection))
        throw std::runtime_error("Can't get connection");

    auto const pixel_format = select_pixel_format(connection);

    auto const mir_egl_app = make_mir_eglapp(connection, pixel_format);

    std::vector<std::shared_ptr<MirEglSurface>> result;

    mir::client::DisplayConfig{connection}.for_each_output([&](MirOutput const* output)
        {
            if (mir_output_get_connection_state(output) == mir_output_connection_state_connected &&
                mir_output_is_enabled(output))
            {
                auto const mode = mir_output_get_current_mode(output);
                auto const output_id = mir_output_get_id(output);

                printf("Active output [%u] at (%d, %d) is %dx%d\n",
                       output_id,
                       mir_output_get_position_x(output), mir_output_get_position_y(output),
                       mir_output_mode_get_width(mode), mir_output_mode_get_height(mode));

                result.push_back(std::make_shared<MirEglSurface>(mir_egl_app, name, output));
            }
        });

    if (result.empty())
        throw std::runtime_error("No active outputs found.");

    return result;
}
