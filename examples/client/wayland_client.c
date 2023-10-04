/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>

#include <wayland-client.h>
#include <wayland-client-core.h>
#include "xdg-shell.h"

static int const pixel_size = 4;
#define NO_OF_BUFFERS 4

static inline uint32_t min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

static struct globals
{
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;
    struct wl_output* output;
    struct xdg_wm_base *xdg_wm_base;
} globals;

void check_globals()
{
    bool fail = false;
    if (!globals.compositor) { puts("ERROR: no wl_compositor*"); fail = true; }
    if (!globals.shm) { puts("ERROR: no wl_shm*"); fail = true; }
    if (!globals.seat) { puts("ERROR: no wl_seat*"); fail = true; }
    if (!globals.output) { puts("ERROR: no wl_output*"); fail = true; }
    if (!globals.xdg_wm_base) { puts("ERROR: no xdg_wm_base*"); fail = true; }

    if (fail) abort();
}

static void handle_xdg_wm_base_ping(void* _, struct xdg_wm_base* shell, uint32_t serial)
{
    (void)_;
    xdg_wm_base_pong(shell, serial);
}

static struct xdg_wm_base_listener const shell_listener =
{
    &handle_xdg_wm_base_ping,
};

static void new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)data;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        globals.compositor = wl_registry_bind(registry, id, &wl_compositor_interface, min(version, 3));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        globals.shm = wl_registry_bind(registry, id, &wl_shm_interface, min(version, 1));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        globals.seat = wl_registry_bind(registry, id, &wl_seat_interface, min(version, 4));
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        globals.output = wl_registry_bind(registry, id, &wl_output_interface, min(version, 2));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        globals.xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, min(version, 1));
        xdg_wm_base_add_listener(globals.xdg_wm_base, &shell_listener, NULL);
    }
}

static void global_remove(
    void* data,
    struct wl_registry* registry,
    uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static struct wl_registry_listener const registry_listener = {
    .global = new_global,
    .global_remove = global_remove
};

static struct wl_shm_pool*
make_shm_pool(struct wl_shm* shm, int size, void **data)
{
    static char* shm_dir = NULL;
    if (!shm_dir) shm_dir = getenv("XDG_RUNTIME_DIR");

    int fd = open(shm_dir, O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU);

    // Workaround for filesystems that don't support O_TMPFILE
    if (fd < 0) {
        char template_filename[] = "/dev/shm/mir-buffer-XXXXXX";
        fd = mkostemp(template_filename, O_CLOEXEC);
        if (fd != -1)
        {
            if (unlink(template_filename) < 0)
            {
                close(fd);
                fd = -1;
            }
        }
    }

    if (fd < 0) {
        return NULL;
    }

    posix_fallocate(fd, 0, size);

    *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    close(fd);

    return pool;
}

static struct wl_buffer_listener const buffer_listener;

typedef struct buffer
{
    struct wl_buffer* buffer;
    bool available;
    int width;
    int height;
    void* content_area;
} buffer;

typedef struct draw_context
{
    struct wl_display* display;
    struct wl_surface* surface;
    struct wl_callback* new_frame_signal;
    int width;
    int height;
    buffer buffers[NO_OF_BUFFERS];
    bool waiting_for_buffer;
} draw_context;

static void prepare_buffer(buffer* b, draw_context* ctx)
{
    void* pool_data = NULL;
    struct wl_shm_pool* shm_pool = make_shm_pool(globals.shm, ctx->width * ctx->height * pixel_size, &pool_data);

    b->buffer = wl_shm_pool_create_buffer(shm_pool, 0, ctx->width, ctx->height, ctx->width*pixel_size, WL_SHM_FORMAT_ARGB8888);
    b->available = true;
    b->width = ctx->width;
    b->height = ctx->height;
    b->content_area = pool_data;
    wl_buffer_add_listener(b->buffer, &buffer_listener, ctx);
    wl_shm_pool_destroy(shm_pool);
}

static buffer*
find_free_buffer(draw_context* ctx)
{
    for (buffer* b = ctx->buffers; b != ctx->buffers + NO_OF_BUFFERS ; ++b)
    {
        if (b->available)
        {
            if (b->width != ctx->width || b->height != ctx->height)
            {
                wl_buffer_destroy(b->buffer);
                prepare_buffer(b, ctx);
            }

            b->available = false;
            return b;
        }
    }
    return NULL;
}

static void draw_new_stuff(void* data, struct wl_callback* callback, uint32_t time);

static const struct wl_callback_listener frame_listener =
{
    .done = &draw_new_stuff
};

static void update_free_buffers(void* data, struct wl_buffer* buffer)
{
    draw_context* ctx = data;
    for (int i = 0; i < NO_OF_BUFFERS ; ++i)
    {
        if (ctx->buffers[i].buffer == buffer)
        {
            ctx->buffers[i].available = true;
        }
    }

    if (ctx->waiting_for_buffer)
    {
        struct wl_callback* fake_frame = wl_display_sync(ctx->display);
        wl_callback_add_listener(fake_frame, &frame_listener, ctx);
    }

    ctx->waiting_for_buffer = false;
}

static void draw_new_stuff(
    void* data,
    struct wl_callback* callback,
    uint32_t time)
{
    (void)time;
    static unsigned char current_value = 128;
    draw_context* ctx = data;

    wl_callback_destroy(callback);

    buffer* ctx_buffer = find_free_buffer(ctx);
    if (!ctx_buffer)
    {
        ctx->waiting_for_buffer = false;
        return;
    }

    memset(ctx_buffer->content_area, current_value, ctx_buffer->width * ctx_buffer->height * pixel_size);
    ++current_value;

    ctx->new_frame_signal = wl_surface_frame(ctx->surface);
    wl_callback_add_listener(ctx->new_frame_signal, &frame_listener, ctx);
    wl_surface_attach(ctx->surface, ctx_buffer->buffer, 0, 0);
    wl_surface_commit(ctx->surface);
}

static struct wl_buffer_listener const buffer_listener = {
    .release = update_free_buffers
};

void mouse_enter(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
    printf("Received mouse_enter event: (%f, %f)\n",
           wl_fixed_to_double(surface_x),
           wl_fixed_to_double(surface_y));
}

void mouse_leave(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    struct wl_surface *surface)
{
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
    printf("Received mouse_exit event\n");
}

void mouse_motion(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    (void)data;
    (void)wl_pointer;

    printf("Received motion event: (%f, %f) @ %i\n",
           wl_fixed_to_double(surface_x),
           wl_fixed_to_double(surface_y),
           time);
}

void mouse_button(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    (void)serial;
    (void)data;
    (void)wl_pointer;

    printf("Received button event: Button %i, state %i @ %i\n",
           button,
           state,
           time);
}

void mouse_axis(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
    (void)data;
    (void)wl_pointer;

    printf("Received axis event: axis %i, value %f @ %i\n",
           axis,
           wl_fixed_to_double(value),
           time);
}



static struct wl_pointer_listener const pointer_listener = {
    .enter = &mouse_enter,
    .leave = &mouse_leave,
    .motion = &mouse_motion,
    .button = &mouse_button,
    .axis = &mouse_axis
};

static void output_geometry(void *data,
                            struct wl_output *wl_output,
                            int32_t x,
                            int32_t y,
                            int32_t physical_width,
                            int32_t physical_height,
                            int32_t subpixel,
                            const char *make,
                            const char *model,
                            int32_t transform)
{
    (void)data;
    (void)wl_output;
    (void)subpixel;
    (void)make;
    (void)model;
    (void)transform;
    printf("Got geometry: (%imm × %imm)@(%i, %i)\n", physical_width, physical_height, x, y);
}

static void output_mode(void *data,
                        struct wl_output *wl_output,
                        uint32_t flags,
                        int32_t width,
                        int32_t height,
                        int32_t refresh)
{
    (void)data;
    (void)wl_output;
    printf("Got mode: %i×%i@%i (flags: %i)\n", width, height, refresh, flags);
}

static void output_done(void* data, struct wl_output* wl_output)
{
    (void)data;
    (void)wl_output;
    printf("Output events done\n");
}

static void output_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    (void)data;
    (void)wl_output;
    printf("Output scale: %i\n", factor);
}

static struct wl_output_listener const output_listener = {
    .geometry = &output_geometry,
    .mode = &output_mode,
    .done = &output_done,
    .scale = &output_scale,
};

static volatile sig_atomic_t running = 0;
static void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

void handle_xdg_surface_configure(void* _, struct xdg_surface* shell_surface, uint32_t serial)
{
    (void)_, (void)shell_surface, (void)serial;
}

static struct xdg_surface_listener const shell_surface_listener =
{
    handle_xdg_surface_configure,
};

void handle_xdg_toplevel_configure(void *data,
                  struct xdg_toplevel *xdg_toplevel,
                  int32_t width_,
                  int32_t height_,
                  struct wl_array *states)
{
    (void)xdg_toplevel, (void)states;
    draw_context* ctx = data;

    if (width_ > 0) ctx->width = width_;
    if (height_ > 0) ctx->height = height_;
}

void handle_xdg_toplevel_close(void *data,
              struct xdg_toplevel *xdg_toplevel)
{
    (void)data, (void)xdg_toplevel;
}

void handle_xdg_toplevel_configure_bounds(void *data,
                         struct xdg_toplevel *xdg_toplevel,
                         int32_t width_,
                         int32_t height_)
{
    (void)data, (void)xdg_toplevel;
    (void)width_, (void)height_;
}

static struct xdg_toplevel_listener const shell_toplevel_listener =
{
    handle_xdg_toplevel_configure,
    handle_xdg_toplevel_close,
    handle_xdg_toplevel_configure_bounds,
};

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    struct wl_display* display = wl_display_connect(NULL);

    struct wl_registry* registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_roundtrip(display);

    check_globals();

    struct wl_pointer* pointer = wl_seat_get_pointer(globals.seat);
    wl_pointer_add_listener(pointer, &pointer_listener, NULL);

    draw_context* ctx = calloc(sizeof *ctx, 1);
    ctx->display = display;
    ctx->surface = wl_compositor_create_surface(globals.compositor);
    ctx->width = 400;
    ctx->height = 400;

    for (buffer* b = ctx->buffers; b != ctx->buffers + NO_OF_BUFFERS ; ++b)
    {
        prepare_buffer(b, ctx);
    }

    struct xdg_surface* shell_surface = xdg_wm_base_get_xdg_surface(globals.xdg_wm_base, ctx->surface);
    xdg_surface_add_listener(shell_surface, &shell_surface_listener, NULL);

    struct xdg_toplevel* shell_toplevel = xdg_surface_get_toplevel(shell_surface);
    xdg_toplevel_add_listener(shell_toplevel, &shell_toplevel_listener, ctx);

    struct wl_callback* first_frame = wl_display_sync(display);
    wl_callback_add_listener(first_frame, &frame_listener, ctx);

    wl_output_add_listener(globals.output, &output_listener, NULL);

    struct sigaction sig_handler_new;
    sigfillset(&sig_handler_new.sa_mask);
    sig_handler_new.sa_flags = 0;
    sig_handler_new.sa_handler = shutdown;

    sigaction(SIGINT, &sig_handler_new, NULL);
    sigaction(SIGTERM, &sig_handler_new, NULL);
    sigaction(SIGHUP, &sig_handler_new, NULL);

    running = 1;

    while (wl_display_dispatch(display) && running)
        ;

    return 0;
}
