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
#include <stdlib.h>

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

struct Card
{
    uint32_t card_id;
    uint32_t available_outputs;
};

struct Cards
{
    size_t num_cards;
    struct Card *cards;
};

static struct Cards *cards_create(struct MirDisplayConfiguration *conf)
{
    struct Cards *cards = (struct Cards*) calloc(1, sizeof(struct Cards));
    cards->num_cards = conf->num_cards;
    cards->cards = (struct Card*) calloc(cards->num_cards, sizeof(struct Card));

    for (size_t i = 0; i < cards->num_cards; i++)
    {
        cards->cards[i].card_id = conf->cards[i].card_id;
        cards->cards[i].available_outputs =
            conf->cards[i].max_simultaneous_outputs;
    }

    return cards;
}

static void cards_free(struct Cards *cards)
{
    free(cards->cards);
    free(cards);
}

static struct Card *cards_find_card(struct Cards *cards, uint32_t card_id)
{
    for (size_t i = 0; i < cards->num_cards; i++)
    {
        if (cards->cards[i].card_id == card_id)
            return &cards->cards[i];
    }

    fprintf(stderr, "Error: Couldn't find card with id: %u\n", card_id);
    abort();
}

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
    struct Cards *cards = cards_create(conf);

    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        struct Card *card = cards_find_card(cards, output->card_id);

        if (output->connected && output->num_modes > 0 &&
            card->available_outputs > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = 0;
            --card->available_outputs;
        }
    }

    cards_free(cards);
}

static void configure_display_horizontal(struct MirDisplayConfiguration *conf)
{
    struct Cards *cards = cards_create(conf);

    uint32_t max_x = 0;
    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        struct Card *card = cards_find_card(cards, output->card_id);

        if (output->connected && output->num_modes > 0 &&
            card->available_outputs > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = max_x;
            output->position_y = 0;
            max_x += output->modes[0].horizontal_resolution;
            --card->available_outputs;
        }
    }

    cards_free(cards);
}

static void configure_display_vertical(struct MirDisplayConfiguration *conf)
{
    struct Cards *cards = cards_create(conf);

    uint32_t max_y = 0;
    for (uint32_t i = 0; i < conf->num_outputs; i++)
    {
        MirDisplayOutput *output = &conf->outputs[i];
        struct Card *card = cards_find_card(cards, output->card_id);

        if (output->connected && output->num_modes > 0 &&
            card->available_outputs > 0)
        {
            output->used = 1;
            output->current_mode = 0;
            output->position_x = 0;
            output->position_y = max_y;
            max_y += output->modes[0].vertical_resolution;
            --card->available_outputs;
        }
    }

    cards_free(cards);
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
        print_current_configuration(ctx->connection);
        break;
    }
}

static void event_callback(
    MirSurface* surface, MirEvent const* event, void* context)
{
    (void) surface;
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

    mir_surface_set_event_handler(surface, event_callback, &ctx);

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
