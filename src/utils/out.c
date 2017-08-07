/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir_toolkit/mir_blob.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

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

static char const* subpixel_name(MirSubpixelArrangement s)
{
    static char const* const name[] =
    {
        "unknown",
        "HRGB",
        "HBGR",
        "VRGB",
        "VBGR",
        "none"
    };
    return ((unsigned)s < sizeof(name)/sizeof(name[0])) ? name[s]
                                                        : "out-of-range";
}

static char const* form_factor_name(MirFormFactor f)
{
    static char const* const name[] =
    {
        "unknown",
        "phone",
        "tablet",
        "monitor",
        "TV",
        "projector",
    };
    return ((unsigned)f < sizeof(name)/sizeof(name[0])) ? name[f]
                                                        : "out-of-range";
}

static bool save(MirDisplayConfig* conf, char const* path)
{
    MirBlob* blob = mir_blob_from_display_config(conf);
    if (!blob)
    {
        fprintf(stderr, "mir_blob_from_display_config failed\n");
        return false;
    }
    bool ret = false;
    FILE* file = !strcmp(path, "-") ? stdout : fopen(path, "wb");
    if (!file)
    {
        perror("save:fopen");
    }
    else
    {
        if (1 == fwrite(mir_blob_data(blob), mir_blob_size(blob), 1, file))
            ret = true;
        else
            perror("save:fwrite");

        if (file != stdout)
            fclose(file);
    }
    mir_blob_release(blob);
    return ret;
}

static MirDisplayConfig* load(char const* path)
{
    MirDisplayConfig* conf = NULL;
    FILE* file = !strcmp(path, "-") ? stdin : fopen(path, "rb");
    if (!file)
    {
        perror("load:fopen");
    }
    else
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        void* data = malloc(size);
        if (!data)
        {
            perror("load:malloc");
        }
        else
        {
            if (1 != fread(data, size, 1, file))
            {
                perror("load:fread");
            }
            else
            {
                MirBlob* blob = mir_blob_onto_buffer(data, size);
                if (!blob)
                {
                    fprintf(stderr, "mir_blob_onto_buffer failed with %ld bytes\n", size);
                }
                else
                {
                    conf = mir_blob_to_display_config(blob);
                    mir_blob_release(blob);
                }
            }
            free(data);
        }
        if (file != stdin)
            fclose(file);
    }
    return conf;
}

static bool modify(MirDisplayConfig* conf, int actionc, char** actionv)
{
    int const max_targets = 256;
    MirOutput* target[max_targets];
    int targets = 0;

    /* Until otherwise specified we apply actions to all outputs */
    int num_outputs = mir_display_config_get_num_outputs(conf);
    if (num_outputs > max_targets)
        num_outputs = max_targets;
    for (int i = 0; i < num_outputs; ++i)
    {
        MirOutput const* out = mir_display_config_get_output(conf, i);
        if (mir_output_connection_state_connected ==
            mir_output_get_connection_state(out))
            target[targets++] = mir_display_config_get_mutable_output(conf, i);
    }

    char** action_end = actionv + actionc;
    for (char** action = actionv; action < action_end; ++action)
    {
        if (!strcmp(*action, "--"))
        {
            /*
             * -- is the only dash option which will and must occur near the
             * end of the command line. So we need to not misinterpret that
             * as a command.
             */
            return true;
        }
        else if (!strcmp(*action, "output"))
        {
            int output_id;
            if (++action < action_end && 1 == sscanf(*action, "%d", &output_id))
            {
                targets = 0;
                for (int i = 0; i < num_outputs; ++i)
                {
                    MirOutput* out =
                        mir_display_config_get_mutable_output(conf, i);
                    if (output_id == mir_output_get_id(out))
                    {
                        targets = 1;
                        target[0] = out;
                        break;
                    }
                }
                if (!targets)
                {
                    fprintf(stderr, "Output ID `%s' not found\n", *action);
                    return false;
                }
            }
            else
            {
                if (action >= action_end)
                    fprintf(stderr, "Missing output ID after `%s'\n",
                            action[-1]);
                else
                    fprintf(stderr, "Invalid output ID `%s'\n", *action);
                return false;
            }
        }
        else if (!strcmp(*action, "off"))
        {
            for (int t = 0; t < targets; ++t)
                mir_output_set_power_mode(target[t], mir_power_mode_off);
        }
        else if (!strcmp(*action, "on"))
        {
            for (int t = 0; t < targets; ++t)
                mir_output_set_power_mode(target[t], mir_power_mode_on);
        }
        else if (!strcmp(*action, "standby"))
        {
            for (int t = 0; t < targets; ++t)
                mir_output_set_power_mode(target[t], mir_power_mode_standby);
        }
        else if (!strcmp(*action, "suspend"))
        {
            for (int t = 0; t < targets; ++t)
                mir_output_set_power_mode(target[t], mir_power_mode_suspend);
        }
        else if (!strcmp(*action, "enable"))
        {
            for (int t = 0; t < targets; ++t)
                mir_output_enable(target[t]);
        }
        else if (!strcmp(*action, "disable"))
        {
            for (int t = 0; t < targets; ++t)
                mir_output_disable(target[t]);
        }
        else if (!strcmp(*action, "rotate"))
        {
            if (++action >= action_end)
            {
                fprintf(stderr, "Missing parameter after `%s'\n", action[-1]);
                return false;
            }
            enum {orientations = 4};
            static const MirOrientation orientation[orientations] =
            {
                mir_orientation_normal,
                mir_orientation_left,
                mir_orientation_inverted,
                mir_orientation_right,
            };

            int i;
            for (i = 0; i < orientations; ++i)
                if (!strcmp(*action, orientation_name(orientation[i])))
                    break;

            if (i >= orientations)
            {
                fprintf(stderr, "Unknown rotation `%s'\n", *action);
                return false;
            }
            else
            {
                for (int t = 0; t < targets; ++t)
                    mir_output_set_orientation(target[t], orientation[i]);
            }
        }
        else if (!strcmp(*action, "place"))
        {
            int x, y;
            if (++action >= action_end)
            {
                fprintf(stderr, "Missing placement parameter after `%s'\n",
                        action[-1]);
                return false;
            }
            else if (2 != sscanf(*action, "%d%d", &x, &y))
            {
                fprintf(stderr, "Invalid placement `%s'\n", *action);
                return false;
            }
            else
            {
                for (int t = 0; t < targets; ++t)
                    mir_output_set_position(target[t], x, y);
            }
        }
        else if (!strcmp(*action, "mode") || !strcmp(*action, "rate"))
        {
            bool have_rate = !strcmp(*action, "rate");
            if (++action >= action_end)
            {
                fprintf(stderr, "Missing parameter after `%s'\n", action[-1]);
                return false;
            }

            int w = -1, h = -1;
            char target_hz[64] = "";

            if (!have_rate)
            {
                if (strcmp(*action, "preferred") &&
                    2 != sscanf(*action, "%dx%d", &w, &h))
                {
                    fprintf(stderr, "Invalid dimensions `%s'\n", *action);
                    return false;
                }

                if (action+2 < action_end && !strcmp(action[1], "rate"))
                {
                    have_rate = true;
                    action += 2;
                }
            }

            if (have_rate)
            {
                if (1 != sscanf(*action, "%63[0-9.]", target_hz))
                {
                    fprintf(stderr, "Invalid refresh rate `%s'\n", *action);
                    return false;
                }
                else if (!strchr(target_hz, '.'))
                {
                    size_t len = strlen(target_hz);
                    if (len < (sizeof(target_hz)-4))
                        snprintf(target_hz+len, 4, ".00");
                }
            }

            for (int t = 0; t < targets; ++t)
            {
                MirOutputMode const* set_mode = NULL;
                MirOutputMode const* preferred =
                    mir_output_get_preferred_mode(target[t]);

                if (w <= 0 && !target_hz[0])
                {
                    set_mode = preferred;
                }
                else
                {
                    if (w <= 0)
                    {
                        w = mir_output_mode_get_width(preferred);
                        h = mir_output_mode_get_height(preferred);
                    }
                    int const num_modes = mir_output_get_num_modes(target[t]);
                    for (int m = 0; m < num_modes; ++m)
                    {
                        MirOutputMode const* mode =
                            mir_output_get_mode(target[t], m);
                        if (w == mir_output_mode_get_width(mode) &&
                            h == mir_output_mode_get_height(mode))
                        {
                            if (!target_hz[0])
                            {
                                set_mode = mode;
                                break;
                            }
                            char hz[64];
                            snprintf(hz, sizeof hz, "%.2f",
                                mir_output_mode_get_refresh_rate(mode));
                            if (!strcmp(target_hz, hz))
                            {
                                set_mode = mode;
                                break;
                            }
                        }
                    }
                }

                if (set_mode)
                {
                    mir_output_set_current_mode(target[t], set_mode);
                    /* Clear the fake mode when a real one is getting set. */
                    mir_output_set_logical_size(target[t], 0, 0);
                }
                else
                {
                    fprintf(stderr, "No matching mode for `%s'\n", *action);
                    return false;
                }
            }
        }
        else if (!strcmp(*action, "fakemode"))
        {
            if (++action >= action_end)
            {
                fprintf(stderr, "Missing parameter after `%s'\n", action[-1]);
                return false;
            }
            unsigned w, h;
            if (2 != sscanf(*action, "%ux%u", &w, &h))
            {
                fprintf(stderr, "Invalid fake resolution `%s'\n", *action);
                return false;
            }
            for (int t = 0; t < targets; ++t)
                mir_output_set_logical_size(target[t], w, h);
        }
        else
        {
            fprintf(stderr, "Unrecognized action `%s'\n", *action);
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    char const* server = NULL;
    char const* infile = NULL;
    char const* outfile = NULL;
    char** actionv = NULL;
    int actionc = 0;
    bool verbose = false;

    for (int a = 1; a < argc; a++)
    {
        const char *arg = argv[a];
        if (arg[0] == '-')
        {
            if (arg[1] == '-' && arg[2] == '\0')
                break;

            switch (arg[1])
            {
                case 'v':
                    verbose = true;
                    break;
                case 'i':
                    ++a;
                    if (a >= argc)
                    {
                        fprintf(stderr, "%s requires a file path\n", arg);
                        return 1;
                    }
                    infile = argv[a];
                    break;
                case 'o':
                    ++a;
                    if (a >= argc)
                    {
                        fprintf(stderr, "%s requires a file path\n", arg);
                        return 1;
                    }
                    outfile = argv[a];
                    break;
                case 'h':
                default:
                    printf("Usage: %s [OPTIONS] [/path/to/mir/socket] [[output OUTPUTID] ACTION ...]\n"
                           "Options:\n"
                           "    -h       Show this help information.\n"
                           "    -i PATH  Load display configuration from a file instead of the server.\n"
                           "    -o PATH  Save resulting configuration to a file.\n"
                           "    -v       Show verbose information.\n"
                           "    --       Ignore the rest of the command line.\n"
                           "Actions:\n"
                           "    off | suspend | standby | on\n"
                           "    disable | enable\n"
                           "    rotate (normal | inverted | left | right)\n"
                           "    place +X+Y\n"
                           "    mode (WIDTHxHEIGHT | preferred) [rate HZ]\n"
                           "    fakemode WIDTHxHEIGHT\n"
                           "    rate HZ\n"
                           , argv[0]);
                    return 0;
            }
        }
        else if (arg[0] == '/')
        {
            server = arg;
        }
        else
        {
            actionv = argv + a;
            actionc = argc - a;
            break;
        }
    }

    MirConnection *conn = mir_connect_sync(server, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        fprintf(stderr, "Could not connect to a display server: %s\n", mir_connection_get_error_message(conn));
        return 1;
    }

    int ret = 0;
    MirDisplayConfig* conf = infile ? load(infile) :
                             mir_connection_create_display_configuration(conn);
    if (conf == NULL)
    {
        fprintf(stderr, "Failed to get display configuration (!?)\n");
    }
    else if (actionc || infile)
    {
        if (modify(conf, actionc, actionv))
        {
            mir_connection_preview_base_display_configuration(conn, conf, 10);
            mir_connection_confirm_base_display_configuration(conn, conf);
        }
        else
        {
            ret = 1;
        }
    }
    else
    {
        printf("Connected to server: %s\n", server ? server : "<default>");

        int num_outputs = mir_display_config_get_num_outputs(conf);

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
                char const* model = mir_output_get_model(out);
                if (model)
                    printf(", \"%s\"", model);

                int physical_width_mm = mir_output_get_physical_width_mm(out);
                int physical_height_mm = mir_output_get_physical_height_mm(out);
                float inches = sqrtf(
                    (physical_width_mm * physical_width_mm) +
                    (physical_height_mm * physical_height_mm))
                    / 25.4f;

                printf(", %ux%u%+d%+d, %s, %s, %dmm x %dmm (%.1f\"), %s, %.2fx, %s, %s",
                       mir_output_get_logical_width(out),
                       mir_output_get_logical_height(out),
                       mir_output_get_position_x(out),
                       mir_output_get_position_y(out),
                       mir_output_is_enabled(out) ? "enabled" : "disabled",
                       power_mode_name(mir_output_get_power_mode(out)),
                       physical_width_mm,
                       physical_height_mm,
                       inches,
                       orientation_name(mir_output_get_orientation(out)),
                       mir_output_get_scale_factor(out),
                       subpixel_name(mir_output_get_subpixel_arrangement(out)),
                       form_factor_name(mir_output_get_form_factor(out)));
            }
            printf("\n");

            /*
             * Note we're not checking if state == connected here but it's
             * probably a good test to probe this stuff unconditionally and
             * make sure it all returns nothing for disconnected outputs...
             */
            uint8_t const* edid = mir_output_get_edid(out);
            if (verbose && edid)
            {
                int indent = 0;
                printf("    EDID: %n", &indent);
                size_t const size = mir_output_get_edid_size(out);
                for (size_t i = 0; i < size; ++i)
                {
                    if (i && (i % 32) == 0)
                    {
                        printf("\n%*c", indent, ' ');
                    }
                    printf("%.2hhx", edid[i]);
                }
                printf("\n");
            }

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
    }

    if (conf)
    {
        if (outfile && !ret)
            ret = save(conf, outfile) ? 0 : 1;
        mir_display_config_release(conf);
    }

    mir_connection_release(conn);

    return ret;
}
