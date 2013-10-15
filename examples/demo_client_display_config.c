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
#include <time.h>

typedef enum
{
    configuration_mode_unknown,
    configuration_mode_clone,
    configuration_mode_horizontal,
    configuration_mode_vertical,
    configuration_mode_single
} ConfigurationMode;

struct ClientContext
{
    MirConnection *connection;
    ConfigurationMode mode;
    int mode_data;
    volatile sig_atomic_t running;
    volatile sig_atomic_t reconfigure;
};

static void print_current_configuration(MirConnection *connection)
{
    MirDisplayConfiguration *conf = mir_connection_create_display_config(connection);

    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        printf("Output id: %d connected: %d used: %d position_x: %d position_y: %d",
               output->output_id, output->connected,
               output->used, output->position_x, output->position_y);
        if (output->current_mode < output->num_modes)
        {
            MirDisplayMode *mode = &output->modes[output->current_mode];
            printf(" mode: %dx%d@%.2f\n",
                   mode->horizontal_resolution,
                   mode->vertical_resolution,
                   mode->refresh_rate);
        }
        else
        {
            printf("\n");
        }
    }

    mir_display_config_destroy(conf);
}

static int apply_configuration(MirConnection *connection, MirDisplayConfiguration *conf)
{
    MirWaitHandle* handle = mir_connection_apply_display_config(connection, conf);
    if (!handle)
    {
        printf("Failed to apply configuration, check that the configuration is valid.\n");
        return 0;
    }

    mir_wait_for(handle);
    const char* error = mir_connection_get_error_message(connection);

    if (!strcmp(error, ""))
    {
        printf("Succeeded.\n");
        return 1;
    }
    else
    {
        printf("Failed to apply configuration with error '%s'.\n", error);
        return 0;
    }
}

static void configure_display_clone(struct MirDisplayConfiguration *conf)
{
    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = 0;
        }
    }
}

static void configure_display_horizontal(struct MirDisplayConfiguration *conf)
{
    uint32_t max_x = 0;
    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = max_x;
            output->position_y = 0;
            max_x += output->modes[0].horizontal_resolution;
        }
    }

}

static void configure_display_vertical(struct MirDisplayConfiguration *conf)
{
    uint32_t max_y = 0;
    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = max_y;
            max_y += output->modes[0].vertical_resolution;
        }
    }
}

static void configure_display_single(struct MirDisplayConfiguration *conf, int output_num)
{
    uint32_t num_connected = 0;
    uint32_t output_to_enable = output_num;

    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        if (output->connected && output->num_modes > 0)
            ++num_connected;
    }

    if (output_to_enable > num_connected)
        output_to_enable = num_connected;

    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        if (output->connected && output->num_modes > 0)
        {
            output->used = (--output_to_enable == 0);
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = 0;
        }
    }
}

static void configure_display(struct ClientContext *context, ConfigurationMode mode,
                              int mode_data)
{
    MirDisplayConfiguration *conf =
        mir_connection_create_display_config(context->connection);

    if (mode == configuration_mode_clone)
    {
        configure_display_clone(conf);
        printf("Applying clone configuration: ");
    }
    else if (mode == configuration_mode_vertical)
    {
        configure_display_vertical(conf);
        printf("Applying vertical configuration: ");
    }
    else if (mode == configuration_mode_horizontal)
    {
        configure_display_horizontal(conf);
        printf("Applying horizontal configuration: ");
    }
    else if (mode == configuration_mode_single)
    {
        configure_display_single(conf, mode_data);
        printf("Applying single configuration for output %d: ", mode_data);
    }

    if (apply_configuration(context->connection, conf))
    {
        context->mode = mode;
        context->mode_data = mode_data;
    }

    mir_display_config_destroy(conf);
}

static void display_change_callback(MirConnection *connection, void *context)
{
    (void)context;

    printf("=== Display configuration changed === \n");

    print_current_configuration(connection);

    struct ClientContext *ctx = (struct ClientContext*) context;
    ctx->reconfigure = 1;
}

static void event_callback(
    MirSurface* surface, MirEvent const* event, void* context)
{
    (void)surface;
    struct ClientContext *ctx = (struct ClientContext*) context;

    if (event->type == mir_event_type_key &&
        event->key.action == mir_key_action_up)
    {
        if (event->key.key_code == XKB_KEY_q)
        {
            ctx->running = 0;
        }
        else if (event->key.key_code == XKB_KEY_c)
        {
            configure_display(ctx, configuration_mode_clone, 0);
        }
        else if (event->key.key_code == XKB_KEY_h)
        {
            configure_display(ctx, configuration_mode_horizontal, 0);
        }
        else if (event->key.key_code == XKB_KEY_v)
        {
            configure_display(ctx, configuration_mode_vertical, 0);
        }
        else if (event->key.key_code >= XKB_KEY_1 &&
                 event->key.key_code <= XKB_KEY_9)
        {
            configure_display(ctx, configuration_mode_single,
                              event->key.key_code - XKB_KEY_0);
        }
        else if (event->key.key_code == XKB_KEY_p)
        {
            print_current_configuration(ctx->connection);
        }
    }
}

int main(int argc, char *argv[])
{
    unsigned int width = 256, height = 256;

    if (!mir_eglapp_init(argc, argv, &width, &height))
    {
        printf("A demo client that allows changing the display configuration. While the client\n"
               "has the focus, use the following keys to change and get information about the\n"
               "display configuration:\n"
               " c: clone outputs\n"
               " h: arrange outputs horizontally in the virtual space\n"
               " v: arrange outputs vertically in the virtual space\n"
               " 1-9: enable only the Nth connected output (in the order returned by the hardware)\n"
               " p: print current display configuration\n");

        return 1;
    }

    MirConnection *connection = mir_eglapp_native_connection();
    MirSurface *surface = mir_eglapp_native_surface();

    struct ClientContext ctx = {connection, configuration_mode_unknown, 0, 1, 0};
    mir_connection_set_display_config_change_callback(
        connection, display_change_callback, &ctx);

    struct MirEventDelegate ed = {event_callback, &ctx};
    mir_surface_set_event_handler(surface, &ed);

    time_t start = time(NULL);

    while (ctx.running && mir_eglapp_running())
    {
        time_t elapsed = time(NULL) - start;
        int mod = elapsed % 3;
        glClearColor(mod == 0 ? 1.0f : 0.0f,
                     mod == 1 ? 1.0f : 0.0f,
                     mod == 2 ? 1.0f : 0.0f,
                     1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();

        if (ctx.reconfigure)
        {
            configure_display(&ctx, ctx.mode, ctx.mode_data);
            ctx.reconfigure = 0;
        }
    }

    mir_eglapp_shutdown();

    return 0;
}
