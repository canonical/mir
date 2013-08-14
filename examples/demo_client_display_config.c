/*
 * Client-side display configuration demo.
 *
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "eglapp.h"
#include "mir_toolkit/mir_client_library.h"

#include <GLES2/gl2.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static volatile sig_atomic_t running = 1;

void apply_configuration(MirConnection *connection, MirDisplayConfiguration *conf)
{
    MirWaitHandle* handle = mir_connection_apply_display_config(connection, conf);
    if (!handle)
    {
        printf("Failed to apply configuration, check that the configuration is valid.\n");
        return;
    }

    mir_wait_for(handle);
    const char* error = mir_connection_get_error_message(connection);

    if (!strcmp(error, ""))
        printf("Succeeded.\n");
    else
        printf("Failed to apply configuration with error '%s'.\n", error);
}

void configure_display_clone(MirConnection *connection)
{
    MirDisplayConfiguration *conf = mir_connection_create_display_config(connection);

    for (uint32_t i = 0; i < conf->num_displays; i++)
    {
        MirDisplayOutput *output = &conf->displays[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = 0;
        }
    }

    printf("Applying clone configuration: ");
    apply_configuration(connection, conf);
    mir_display_config_destroy(conf);
}

void configure_display_horizontal(MirConnection *connection)
{
    MirDisplayConfiguration *conf = mir_connection_create_display_config(connection);

    uint32_t max_x = 0;
    for (uint32_t i = 0; i < conf->num_displays; i++)
    {
        MirDisplayOutput *output = &conf->displays[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = max_x;
            output->position_y = 0;
            max_x += output->modes[0].horizontal_resolution;
        }
    }

    printf("Applying horizontal configuration: ");
    apply_configuration(connection, conf);
    mir_display_config_destroy(conf);
}

void configure_display_vertical(MirConnection *connection)
{
    MirDisplayConfiguration *conf = mir_connection_create_display_config(connection);

    uint32_t max_y = 0;
    for (uint32_t i = 0; i < conf->num_displays; i++)
    {
        MirDisplayOutput *output = &conf->displays[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = max_y;
            max_y += output->modes[0].vertical_resolution;
        }
    }

    printf("Applying vertical configuration: ");
    apply_configuration(connection, conf);
    mir_display_config_destroy(conf);
}

void display_change_callback(MirConnection *connection, void *context)
{
    (void)context;

    printf("=== Display configuration changed === \n");

    MirDisplayConfiguration *conf = mir_connection_create_display_config(connection);

    for (uint32_t i = 0; i < conf->num_displays; i++)
    {
        MirDisplayOutput *output = &conf->displays[i];
        printf("Output id: %d connected: %d used: %d position_x: %d position_y: %d\n",
               output->output_id, output->connected,
               output->used, output->position_x, output->position_y);
    }

    mir_display_config_destroy(conf);
}

void event_callback(
    MirSurface* surface, MirEvent const* event, void* context)
{
    (void)surface;
    MirConnection *connection = (MirConnection*) context;

    if (event->type == mir_event_type_key &&
        event->key.action == mir_key_action_up)
    {
        if (event->key.key_code == XKB_KEY_q)
        {
            running = 0;
        }
        else if (event->key.key_code == XKB_KEY_c)
        {
            configure_display_clone(connection);
        }
        else if (event->key.key_code == XKB_KEY_h)
        {
            configure_display_horizontal(connection);
        }
        else if (event->key.key_code == XKB_KEY_v)
        {
            configure_display_vertical(connection);
        }
    }
}


int main(int argc, char *argv[])
{
    unsigned int width = 256, height = 256;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    MirConnection *connection = mir_eglapp_native_connection();
    MirSurface *surface = mir_eglapp_native_surface();

    mir_connection_set_display_config_change_callback(
        connection, display_change_callback, NULL);

    struct MirEventDelegate ed = { event_callback, connection };
    mir_surface_set_event_handler(surface, &ed);

    while (running && mir_eglapp_running())
    {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);

        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);

        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);
    }

    mir_eglapp_shutdown();

    return 0;
}
