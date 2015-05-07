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

static const char *output_type_name(MirDisplayOutputType t)
{
    /* XXX it would be safer to define these strings in the client header */
    static const char * const name[] =
    {
        "unknown",
        "VGA",
        "DVI-I",
        "DVI-D",
        "DVI-A",
        "Composite",
        "S-video",
        "LVDS",
        "Component",
        "9-pin",
        "DisplayPort",
        "HDMI-A",
        "HDMI-B",
        "TV",
        "eDP",
    };
    return ((unsigned)t < sizeof(name)/sizeof(name[0])) ? name[t] : name[0];
}

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

int main(int argc, char *argv[])
{
    const char *server = NULL;

    for (int a = 1; a < argc; a++)
    {
        const char *arg = argv[a];
        if (arg[0] == '-')
        {
            switch (arg[1])
            {
                case 'h':
                default:
                    printf("Usage: %s [-h] [<socket-file>]\n"
                           "Options:\n"
                           "    -h  Show this help information.\n"
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

    MirDisplayConfiguration *conf = mir_connection_create_display_config(conn);
    if (conf == NULL)
    {
        fprintf(stderr, "Failed to get display configuration (!?)\n");
    }
    else
    {
        for (unsigned c = 0; c < conf->num_cards; ++c)
        {
            const MirDisplayCard *card = conf->cards + c;
            printf("Card %u: Max %u simultaneous outputs\n",
                   card->card_id, card->max_simultaneous_outputs);
        }

        for (unsigned i = 0; i < conf->num_outputs; ++i)
        {
            const MirDisplayOutput *out = conf->outputs + i;

            printf("Output %u: Card %u, %s, %s",
                   out->output_id,
                   out->card_id,
                   output_type_name(out->type),
                   out->connected ? "connected" : "disconnected");

            if (out->connected)
            {
                if (out->current_mode < out->num_modes)
                {
                    const MirDisplayMode *cur = out->modes + out->current_mode;
                    printf(", %ux%u",
                           cur->horizontal_resolution,
                           cur->vertical_resolution);
                }
                else
                {
                    printf(", ");
                }

                float inches = sqrtf(
                    (out->physical_width_mm * out->physical_width_mm) +
                    (out->physical_height_mm * out->physical_height_mm))
                    / 25.4f;

                printf("%+d%+d, %s, %s, %umm x %umm (%.1f\"), %s",
                       out->position_x,
                       out->position_y,
                       out->used ? "used" : "unused",
                       power_mode_name(out->power_mode),
                       out->physical_width_mm,
                       out->physical_height_mm,
                       inches,
                       orientation_name(out->orientation));
            }
            printf("\n");

            for (unsigned m = 0; m < out->num_modes; ++m)
            {
                const MirDisplayMode *mode = out->modes + m;

                if (m == 0 ||
                    mode->horizontal_resolution !=
                        mode[-1].horizontal_resolution ||
                    mode->vertical_resolution !=
                        mode[-1].vertical_resolution)
                {
                    if (m)
                        printf("\n");

                    printf("%8ux%-8u",
                           mode->horizontal_resolution,
                           mode->vertical_resolution);
                }

                printf("%6.2f%c%c",
                       mode->refresh_rate,
                       (m == out->current_mode) ? '*' : ' ',
                       (m == out->preferred_mode) ? '+' : ' ');
            }

            if (out->num_modes)
                printf("\n");
        }

        mir_display_config_destroy(conf);
    }

    mir_connection_release(conn);

    return 0;
}
