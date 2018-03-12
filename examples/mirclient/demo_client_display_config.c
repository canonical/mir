/*
 * Client-side display configuration demo.
 *
 * Copyright © 2013, 2017 Canonical Ltd.
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
#include <stdlib.h>

typedef enum
{
    configuration_mode_unknown,
    configuration_mode_clone,
    configuration_mode_horizontal,
    configuration_mode_vertical,
    configuration_mode_single,
    configuration_mode_left,
    configuration_mode_right,
    configuration_mode_up,
    configuration_mode_down,
} ConfigurationMode;

struct ClientContext
{
    MirConnection *connection;
    ConfigurationMode mode;
    int mode_data;
    volatile sig_atomic_t running;
    volatile sig_atomic_t reconfigure;
};

static MirDisplayConfig *conf = NULL;

static void print_current_configuration()
{
    if (!conf)
        return;

    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (uint32_t i = 0; i < num_outputs; i++)
    {
        MirOutput const* output = mir_display_config_get_output(conf, i);
        int id = mir_output_get_id(output);
        int position_x = mir_output_get_position_x(output);
        int position_y = mir_output_get_position_y(output);
        bool used = mir_output_is_enabled(output);
        bool connected = mir_output_get_connection_state(output) ==
            mir_output_connection_state_connected;

        printf("Output id: %d connected: %d used: %d position_x: %d position_y: %d orientation: %d",
               id, connected, used, position_x, position_y, mir_output_get_orientation(output));

        MirOutputMode const* current = mir_output_get_current_mode(output);
        if (current)
        {
            printf(" mode: %dx%d@%.2f\n",
                mir_output_mode_get_width(current),
                mir_output_mode_get_height(current),
                mir_output_mode_get_refresh_rate(current));
        }
        else
        {
            printf("\n");
        }
    }
}

static int apply_configuration(MirConnection *connection, MirDisplayConfig *conf)
{
    mir_connection_apply_session_display_config(connection, conf);
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

static void configure_display_clone(MirDisplayConfig *conf)
{
    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput* output = mir_display_config_get_mutable_output(conf, i);
        size_t num_modes = mir_output_get_num_modes(output);
        int state = mir_output_get_connection_state(output);

        if (state == mir_output_connection_state_connected &&
            num_modes > 0)
        {
            mir_output_enable(output);
        }
    }
}

static void configure_display_horizontal(MirDisplayConfig *conf)
{
    uint32_t max_x = 0;
    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput* output = mir_display_config_get_mutable_output(conf, i);
        size_t num_modes = mir_output_get_num_modes(output);
        int state = mir_output_get_connection_state(output);

        if (state == mir_output_connection_state_connected &&
            num_modes > 0)
        {
            mir_output_enable(output);
            mir_output_set_position(output, max_x, 0);

            MirOutputMode const* current = mir_output_get_current_mode(output);
            max_x += mir_output_mode_get_width(current);
        }
    }
}

static void configure_display_vertical(MirDisplayConfig *conf)
{
    uint32_t max_y = 0;
    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput* output = mir_display_config_get_mutable_output(conf, i);
        size_t num_modes = mir_output_get_num_modes(output);
        int state = mir_output_get_connection_state(output);

        if (state == mir_output_connection_state_connected &&
            num_modes > 0)
        {
            mir_output_enable(output);
            mir_output_set_position(output, 0, max_y);

            MirOutputMode const* current = mir_output_get_current_mode(output);
            max_y += mir_output_mode_get_height(current);
        }
    }
}

static void configure_display_single(MirDisplayConfig *conf, int output_num)
{
    uint32_t num_connected = 0;
    uint32_t output_to_enable = output_num;
    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput const* output = mir_display_config_get_output(conf, i);
        size_t num_modes = mir_output_get_num_modes(output);
        int state = mir_output_get_connection_state(output);

        if (state == mir_output_connection_state_connected &&
            num_modes > 0)
        {
            ++num_connected;
        }
    }

    if (output_to_enable > num_connected)
        output_to_enable = num_connected;

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput *output = mir_display_config_get_mutable_output(conf, i);
        size_t num_modes = mir_output_get_num_modes(output);
        int state = mir_output_get_connection_state(output);

        if (state == mir_output_connection_state_connected &&
            num_modes > 0)
        {
            if (--output_to_enable == 0)
            {
                mir_output_enable(output);
            }
            else
            {
                mir_output_disable(output);
            }
        }
    }
}

static void orient_display(MirDisplayConfig *conf, MirOrientation orientation)
{
    size_t num_outputs = mir_display_config_get_num_outputs(conf);

    for (size_t i = 0; i < num_outputs; i++)
    {
        MirOutput *output = mir_display_config_get_mutable_output(conf, i);
        mir_output_set_orientation(output, orientation);
    }
}

static void configure_display(struct ClientContext *context, ConfigurationMode mode,
                              int mode_data)
{
    if (!conf)
        conf = mir_connection_create_display_configuration(context->connection);

    switch (mode)
    {
    case configuration_mode_clone:
        configure_display_clone(conf);
        printf("Applying clone configuration: ");
        break;

    case configuration_mode_vertical:
        configure_display_vertical(conf);
        printf("Applying vertical configuration: ");
        break;

    case configuration_mode_horizontal:
        configure_display_horizontal(conf);
        printf("Applying horizontal configuration: ");
        break;

    case configuration_mode_single:
        configure_display_single(conf, mode_data);
        printf("Applying single configuration for output %d: ", mode_data);
        break;

    case configuration_mode_left:
        orient_display(conf, mir_orientation_left);
        printf("Applying orientation left: ");
        break;

    case configuration_mode_right:
        orient_display(conf, mir_orientation_right);
        printf("Applying orientation right: ");
        break;

    case configuration_mode_up:
        orient_display(conf, mir_orientation_inverted);
        printf("Applying orientation up: ");
        break;

    case configuration_mode_down:
        orient_display(conf, mir_orientation_normal);
        printf("Applying orientation down: ");
        break;

    default:
        break;
    }

    if (apply_configuration(context->connection, conf))
    {
        context->mode = mode;
        context->mode_data = mode_data;
    }
}

static void display_change_callback(MirConnection *connection, void *context)
{
    printf("=== Display configuration changed === \n");

    if (conf)
        mir_display_config_release(conf);

    conf = mir_connection_create_display_configuration(connection);

    print_current_configuration();

    struct ClientContext *ctx = (struct ClientContext*) context;
    ctx->reconfigure = 1;
}

static void apply_to_base_configuration(MirConnection *connection)
{
    if (!conf)
        return;

    mir_connection_preview_base_display_configuration(connection, conf, 2);
    puts("Applying to base configuration");
    mir_connection_confirm_base_display_configuration(connection, conf);
}

static void handle_keyboard_event(struct ClientContext *ctx, MirKeyboardEvent const* event)
{
    if (mir_keyboard_event_action(event) != mir_keyboard_action_up)
        return;
    xkb_keysym_t key_code = mir_keyboard_event_key_code(event);

    if (key_code >= XKB_KEY_1 &&
        key_code <= XKB_KEY_9)
    {
        configure_display(ctx, configuration_mode_single,
                          key_code - XKB_KEY_0);
    }

    switch (key_code)
    {
    case XKB_KEY_a:
        apply_to_base_configuration(ctx->connection);
        break;
    case XKB_KEY_q:
        ctx->running = 0;
        break;
    case XKB_KEY_c:
        configure_display(ctx, configuration_mode_clone, 0);
        break;
    case XKB_KEY_h:
        configure_display(ctx, configuration_mode_horizontal, 0);
        break;
    case XKB_KEY_v:
        configure_display(ctx, configuration_mode_vertical, 0);
        break;
    case XKB_KEY_p:
        if (!conf)
            conf = mir_connection_create_display_configuration(ctx->connection);
        print_current_configuration();
        break;
    case XKB_KEY_Left:
        configure_display(ctx, configuration_mode_right, 0);
        break;
    case XKB_KEY_Up:
        configure_display(ctx, configuration_mode_up, 0);
        break;
    case XKB_KEY_Right:
        configure_display(ctx, configuration_mode_left, 0);
        break;
    case XKB_KEY_Down:
        configure_display(ctx, configuration_mode_down, 0);
        break;
    }
}

static void event_callback(
    MirWindow* surface, MirEvent const* event, void* context)
{
    mir_eglapp_handle_event(surface, event, context);

    struct ClientContext *ctx = (struct ClientContext*) context;

    if (mir_event_get_type(event) != mir_event_type_input)
        return;
    MirInputEvent const* input_event = mir_event_get_input_event(event);
    if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
        return;

    handle_keyboard_event(ctx, mir_input_event_get_keyboard_event(input_event));
}

int main(int argc, char *argv[])
{
    unsigned int width = 256, height = 256;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
    {
        printf("A demo client that allows changing the display configuration. While the client\n"
               "has the focus, use the following keys to change and get information about the\n"
               "display configuration:\n"
               " c: clone outputs\n"
               " h: arrange outputs horizontally in the virtual space\n"
               " v: arrange outputs vertically in the virtual space\n"
               " 1-9: enable only the Nth connected output (in the order returned by the hardware)\n"
               " Arrows: orient display (sets \"down\" direction)\n"
               " p: print current display configuration\n"
               " a: apply current display configuration globally\n");

        return 1;
    }

    MirConnection* connection = mir_eglapp_native_connection();
    MirWindow* window = mir_eglapp_native_window();

    struct ClientContext ctx = {connection, configuration_mode_unknown, 0, 1, 0};
    mir_connection_set_display_config_change_callback(
        connection, display_change_callback, &ctx);

    mir_window_set_event_handler(window, event_callback, &ctx);

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

    if (conf)
        mir_display_config_release(conf);

    mir_eglapp_cleanup();

    return 0;
}
