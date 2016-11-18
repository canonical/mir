/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include <stdio.h>
#include <math.h>

static const char *power_mode_name(MirPowerMode m)
{
    /* XXX it would be safer to define these strings in the client header */
    static const char * const name[] =
    {
        "on",
        "standby",
        "suspended",
        "off"
    };
    return ((unsigned)m < sizeof(name)/sizeof(name[0])) ? name[m] : "unknown";
}

static const char *orientation_name(MirOrientation ori)
{
    static const char * const name[] =
    {
        "normal",
        "left",
        "inverted",
        "right"
    };
    return name[(ori % 360) / 90];
}

static char const* state_name(MirOutputConnectionState s)
{
    static char const* const name[] =
    {
        "disconnected",
        "connected",
        "unknown",
    };
    unsigned int u = s;
    return u < 3 ? name[u] : "out-of-range";
}

int main(int argc, char *argv[])
{
    const char *server = NULL;

    for (int a = 1; a < argc; a++)
    {
        const char *arg = argv[a];
        if (arg[0] == '-')
        {
            if (arg[1] == '-' && arg[2] == '\0')
                break;

            switch (arg[1])
            {
                case 'h':
                default:
                    printf("Usage: %s [-h] [<socket-file>] [--]\n"
                           "Options:\n"
                           "    -h  Show this help information.\n"
                           "    --  Ignore the rest of the command line.\n"
                           , argv[0]);
                    return 0;
            }
        }
        else
        {
            server = arg;
        }
    }

    MirConnection *conn = mir_connect_sync(server, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server: %s\n", mir_connection_get_error_message(conn));
        return 1;
    }

    printf("Connected to server: %s\n", server ? server : "<default>");

    MirDisplayConfig* conf = mir_connection_create_display_configuration(conn);
    if (conf == NULL)
    {
        fprintf(stderr, "Failed to get display configuration (!?)\n");
    }
    else
    {
        int num_outputs = mir_display_config_get_num_outputs(conf);

        printf("Max %d simultaneous outputs\n",
               mir_display_config_get_max_simultaneous_outputs(conf));

        for (int i = 0; i < num_outputs; ++i)
        {
            MirOutput const* out = mir_display_config_get_output(conf, i);
            MirOutputConnectionState const state =
                mir_output_get_connection_state(out);

            printf("Output %d: %s, %s",
                   mir_output_get_id(out),
                   mir_output_type_name(mir_output_get_type(out)),
                   state_name(state));

            if (state == mir_output_connection_state_connected)
            {
                MirOutputMode const* current_mode =
                    mir_output_get_current_mode(out);
                if (current_mode)
                {
                    printf(", %dx%d",
                           mir_output_mode_get_width(current_mode),
                           mir_output_mode_get_height(current_mode));
                }
                else
                {
                    printf(", ");
                }

                int physical_width_mm = mir_output_get_physical_width_mm(out);
                int physical_height_mm = mir_output_get_physical_height_mm(out);
                float inches = sqrtf(
                    (physical_width_mm * physical_width_mm) +
                    (physical_height_mm * physical_height_mm))
                    / 25.4f;

                printf("%+d%+d, %s, %s, %dmm x %dmm (%.1f\"), %s",
                       mir_output_get_position_x(out),
                       mir_output_get_position_y(out),
                       mir_output_is_enabled(out) ? "enabled" : "disabled",
                       power_mode_name(mir_output_get_power_mode(out)),
                       physical_width_mm,
                       physical_height_mm,
                       inches,
                       orientation_name(mir_output_get_orientation(out)));
            }
            printf("\n");

            int const num_modes = mir_output_get_num_modes(out);
            int const current_mode_index =
                mir_output_get_current_mode_index(out);
            int const preferred_mode_index =
                mir_output_get_preferred_mode_index(out);
            int prev_width = -1, prev_height = -1;

            for (int m = 0; m < num_modes; ++m)
            {
                MirOutputMode const* mode = mir_output_get_mode(out, m);
                int const width = mir_output_mode_get_width(mode);
                int const height = mir_output_mode_get_height(mode);

                if (m == 0 || width != prev_width || height != prev_height)
                {
                    if (m)
                        printf("\n");

                    printf("%8dx%-8d", width, height);
                }

                printf("%6.2f%c%c",
                       mir_output_mode_get_refresh_rate(mode),
                       (m == current_mode_index) ? '*' : ' ',
                       (m == preferred_mode_index) ? '+' : ' ');

                prev_width = width;
                prev_height = height;
            }

            if (num_modes)
                printf("\n");
        }

        mir_display_config_release(conf);
    }

    mir_connection_release(conn);

    return 0;
}
