/*
 * Copyright © 2016-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "sw_splash.h"

#include <wayland-client.h>
#include <wayland-client-core.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <mutex>

namespace
{
struct wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data)
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

struct globals
{
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_seat* seat;
    struct wl_output* output;
    struct wl_shell* shell;
};

void new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)version;
    struct globals* globals = static_cast<decltype(globals)>(data);

    if (strcmp(interface, "wl_compositor") == 0)
    {
        globals->compositor =
            static_cast<decltype(globals->compositor)>(wl_registry_bind(registry, id, &wl_compositor_interface, 3));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        globals->shm = static_cast<decltype(globals->shm)>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        globals->seat = static_cast<decltype(globals->seat)>(wl_registry_bind(registry, id, &wl_seat_interface, 4));
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        globals->output = static_cast<decltype(globals->output)>(wl_registry_bind(registry, id, &wl_output_interface, 2));
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        globals->shell = static_cast<decltype(globals->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
    }
}

void global_remove(
    void* data,
    struct wl_registry* registry,
    uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

wl_registry_listener const registry_listener = {
    new_global,
    global_remove
};

void draw_new_stuff(void* data, struct wl_callback* callback, uint32_t time);

wl_callback_listener const frame_listener =
{
    &draw_new_stuff
};

struct draw_context
{
    void* content_area;
    struct wl_display* display;
    struct wl_surface* surface;
    struct wl_callback* new_frame_signal;
    struct Buffers
    {
        struct wl_buffer* buffer;
        bool available;
    } buffers[4];
    bool waiting_for_buffer;
};

wl_buffer* find_free_buffer(struct draw_context* ctx)
{
    for (int i = 0; i < 4 ; ++i)
    {
        if (ctx->buffers[i].available)
        {
            ctx->buffers[i].available = false;
            return ctx->buffers[i].buffer;
        }
    }
    return NULL;
}

void draw_new_stuff(
    void* data,
    struct wl_callback* callback,
    uint32_t time)
{
    (void)time;
    static unsigned char current_value = 128;
    struct draw_context* ctx = static_cast<decltype(ctx)>(data);

    wl_callback_destroy(callback);

    struct wl_buffer* buffer = find_free_buffer(ctx);
    if (!buffer)
    {
        ctx->waiting_for_buffer = false;
        return;
    }

    memset(ctx->content_area, current_value, 400 * 400 * 4);
    ++current_value;

    ctx->new_frame_signal = wl_surface_frame(ctx->surface);
    wl_callback_add_listener(ctx->new_frame_signal, &frame_listener, ctx);
    wl_surface_attach(ctx->surface, buffer, 0, 0);
    wl_surface_commit(ctx->surface);
}

void update_free_buffers(void* data, struct wl_buffer* buffer)
{
    struct draw_context* ctx = static_cast<decltype(ctx)>(data);
    for (int i = 0; i < 4 ; ++i)
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

wl_buffer_listener const buffer_listener = {
    update_free_buffers
};

void output_geometry(void *data,
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

void output_mode(void *data,
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

void output_done(void* data, struct wl_output* wl_output)
{
    (void)data;
    (void)wl_output;
    printf("Output events done\n");
}

void output_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    (void)data;
    (void)wl_output;
    printf("Output scale: %i\n", factor);
}

wl_output_listener const output_listener = {
    &output_geometry,
    &output_mode,
    &output_done,
    &output_scale,
};

//MirPixelFormat find_8888_format(MirConnection* connection)
//{
//    unsigned int const num_formats = 32;
//    MirPixelFormat pixel_formats[num_formats];
//    unsigned int valid_formats;
//    mir_connection_get_available_surface_formats(connection, pixel_formats, num_formats, &valid_formats);
//
//    for (unsigned int i = 0; i < num_formats; ++i)
//    {
//        MirPixelFormat cur_pf = pixel_formats[i];
//        if (cur_pf == mir_pixel_format_abgr_8888 ||
//            cur_pf == mir_pixel_format_argb_8888)
//        {
//            return cur_pf;
//        }
//    }
//
//    for (unsigned int i = 0; i < num_formats; ++i)
//    {
//        MirPixelFormat cur_pf = pixel_formats[i];
//        if (cur_pf == mir_pixel_format_xbgr_8888 ||
//            cur_pf == mir_pixel_format_xrgb_8888)
//        {
//            return cur_pf;
//        }
//    }
//
//    return *pixel_formats;
//}

//auto create_window(MirConnection* connection, mir::client::Surface const& surface) -> mir::client::Window
//{
//    int id = 0;
//    int width = 0;
//    int height = 0;
//
//    mir::client::DisplayConfig{connection}.for_each_output([&](MirOutput const* output)
//        {
//            if (mir_output_get_connection_state(output) == mir_output_connection_state_connected &&
//                mir_output_is_enabled(output))
//            {
//                id = mir_output_get_id(output);
//                width = mir_output_get_logical_width(output);
//                height = mir_output_get_logical_height(output);
//            }
//        });
//
//    return mir::client::WindowSpec::for_normal_window(connection, width, height)
//        .set_name("splash")
//        .set_fullscreen_on_output(id)
//        .add_surface(surface, width, height, 0, 0)
//        .create_window();
//}

//void render_pattern(MirGraphicsRegion *region, uint8_t pattern[])
//{
//    char *row = region->vaddr;
//
//    for (int j = 0; j < region->height; j++)
//    {
//        uint32_t *pixel = (uint32_t*)row;
//
//        for (int i = 0; i < region->width; i++)
//            memcpy(pixel+i, pattern, sizeof pixel[i]);
//
//        row += region->stride;
//    }
//}
}

struct SwSplash::Self : SplashSession
{
    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> session_;

    std::shared_ptr<mir::scene::Session> session() const override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return session_.lock();
    }
};

SwSplash::SwSplash() : self{std::make_shared<Self>()} {}

SwSplash::~SwSplash() = default;

void SwSplash::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    std::lock_guard<decltype(self->mutex)> lock{self->mutex};
    self->session_ = session;
}

SwSplash::operator std::shared_ptr<SplashSession>() const
{
    return self;
}

void SwSplash::operator()(struct wl_display* display)
{
    struct globals* globals;
    globals = new struct globals;

    struct wl_registry* registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, globals);

    wl_display_roundtrip(display);

    void* pool_data = NULL;
    struct wl_shm_pool* shm_pool = make_shm_pool(globals->shm, 400 * 400 * 4, &pool_data);

    struct draw_context* ctx = new draw_context;

    for (int i = 0; i < 4; ++i)
    {
        ctx->buffers[i].buffer = wl_shm_pool_create_buffer(shm_pool, 0, 400, 400, 400*4, WL_SHM_FORMAT_ARGB8888);
        ctx->buffers[i].available = true;
        wl_buffer_add_listener(ctx->buffers[i].buffer, &buffer_listener, ctx);
    }

    ctx->display = display;
    ctx->surface = wl_compositor_create_surface(globals->compositor);
    ctx->content_area = pool_data;

    struct wl_shell_surface* window = wl_shell_get_shell_surface(globals->shell, ctx->surface);
    wl_shell_surface_set_toplevel(window);

    struct wl_callback* first_frame = wl_display_sync(display);
    wl_callback_add_listener(first_frame, &frame_listener, ctx);

    wl_output_add_listener(globals->output, &output_listener, NULL);

    auto const time_limit = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    while (wl_display_dispatch(display) && std::chrono::steady_clock::now() < time_limit)
        ;
}

//void SwSplash::operator()(MirConnection* connection)
//{
//    MirPixelFormat pixel_format = find_8888_format(connection);
//
//    uint8_t pattern[4] = { 0x14, 0x48, 0xDD, 0xFF };
//
//    switch(pixel_format)
//    {
//    case mir_pixel_format_abgr_8888:
//    case mir_pixel_format_xbgr_8888:
//        std::swap(pattern[2],pattern[0]);
//        break;
//
//    case mir_pixel_format_argb_8888:
//    case mir_pixel_format_xrgb_8888:
//        break;
//
//    default:
//        return;
//    };
//
//
//    mir::client::Surface surface{mir_connection_create_render_surface_sync(connection, 42, 42)};
//    MirBufferStream* buffer_stream = mir_render_surface_get_buffer_stream(surface, 42, 42, pixel_format);
//
//    auto const window = create_window(connection, surface);
//
//    MirGraphicsRegion graphics_region;
//
//    auto const time_limit = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//
//    do
//    {
//        mir_buffer_stream_get_graphics_region(buffer_stream, &graphics_region);
//
//        render_pattern(&graphics_region, pattern);
//        mir_buffer_stream_swap_buffers_sync(buffer_stream);
//
//        for (auto& x : pattern)
//            x =  3*x/4;
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(200));
//    }
//    while (std::chrono::steady_clock::now() < time_limit);
//}
