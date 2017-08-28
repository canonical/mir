/*
 * Copyright © 2015 Canonical LTD
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
 * Author: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <wayland-client.h>

struct globals
{
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;
    struct wl_output* output;
};

static void new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)version;
    struct globals* globals = data;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        globals->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 3);
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        globals->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        globals->seat = wl_registry_bind(registry, id, &wl_seat_interface, 4);
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        globals->output = wl_registry_bind(registry, id, &wl_output_interface, 2);
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
    struct wl_shm_pool *pool;
    int fd;

    fd = open("/dev/shm", O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU);
    if (fd < 0) {
        return NULL;
    }

    posix_fallocate(fd, 0, size);

    *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    pool = wl_shm_create_pool(shm, fd, size);

    close(fd);

    return pool;
}

struct draw_context
{
    void* content_area;
    struct wl_surface* surface;
};

static void draw_new_stuff(void* data, struct wl_buffer* buffer)
{
    static unsigned char current_value = 0;
    struct draw_context* ctx = data;

    memset(ctx->content_area, current_value, 400 * 400 * 4);
    ++current_value;

    wl_surface_attach(ctx->surface, buffer, 0, 0);
    wl_surface_commit(ctx->surface);
}

static struct wl_buffer_listener const buffer_listener = {
    .release = draw_new_stuff
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

int main()
{
    struct wl_display* display = wl_display_connect(NULL);
    struct globals* globals;
    globals = calloc(sizeof *globals, 1);

    struct wl_registry* registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, globals);

    wl_display_roundtrip(display);

    struct wl_pointer* pointer = wl_seat_get_pointer(globals->seat);
    wl_pointer_add_listener(pointer, &pointer_listener, NULL);

    void* pool_data = NULL;
    struct wl_shm_pool* shm_pool = make_shm_pool(globals->shm, 400 * 400 * 4, &pool_data);

    struct wl_buffer* buffer = wl_shm_pool_create_buffer(shm_pool, 0, 400, 400, 400, WL_SHM_FORMAT_ARGB8888);

    struct draw_context* ctx = calloc(sizeof *ctx, 1);
    ctx->surface = wl_compositor_create_surface(globals->compositor);
    ctx->content_area = pool_data;

    draw_new_stuff(ctx, buffer);

    wl_buffer_add_listener(buffer, &buffer_listener, ctx);

    wl_output_add_listener(globals->output, &output_listener, NULL);

    while (wl_display_dispatch(display))
        ;

    return 0;
}
