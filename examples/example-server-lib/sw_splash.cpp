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
#include <vector>

namespace
{
struct DrawContext
{
    int width = 400;
    int height = 400;

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

    uint8_t pattern[4] = { 0x14, 0x48, 0xDD, 0xFF };
};
}

struct SwSplash::Self : SplashSession
{
    struct wl_compositor* compositor = nullptr;
    struct wl_shm* shm = nullptr;
    struct wl_seat* seat = nullptr;
    struct wl_output* output = nullptr;
    struct wl_shell* shell = nullptr;

    struct OutputInfo
    {
        struct wl_output* wl_output;
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    };

    std::vector<OutputInfo> output_info;

    DrawContext ctx;

    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> session_;

    std::shared_ptr<mir::scene::Session> session() const override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return session_.lock();
    }
};

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

void new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)version;
    SwSplash::Self* self = static_cast<decltype(self)>(data);

    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor =
            static_cast<decltype(self->compositor)>(wl_registry_bind(registry, id, &wl_compositor_interface, 3));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        self->shm = static_cast<decltype(self->shm)>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        self->seat = static_cast<decltype(self->seat)>(wl_registry_bind(registry, id, &wl_seat_interface, 4));
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        self->output = static_cast<decltype(self->output)>(wl_registry_bind(registry, id, &wl_output_interface, 2));
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        self->shell = static_cast<decltype(self->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
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

wl_buffer* find_free_buffer(DrawContext* ctx)
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
    uint32_t /*time*/)
{
    DrawContext* ctx = static_cast<decltype(ctx)>(data);

    wl_callback_destroy(callback);

    struct wl_buffer* buffer = find_free_buffer(ctx);
    if (!buffer)
    {
        ctx->waiting_for_buffer = false;
        return;
    }

    char* row = static_cast<decltype(row)>(ctx->content_area);
    for (int j = 0; j < ctx->height; j++)
    {
        uint32_t* pixel = (uint32_t*)row;

        for (int i = 0; i < ctx->width; i++)
            memcpy(pixel + i, ctx->pattern, sizeof pixel[i]);

        row += 4*ctx->width;
    }

    ctx->new_frame_signal = wl_surface_frame(ctx->surface);
    wl_callback_add_listener(ctx->new_frame_signal, &frame_listener, ctx);
    wl_surface_attach(ctx->surface, buffer, 0, 0);
    wl_surface_commit(ctx->surface);
}

void update_free_buffers(void* data, struct wl_buffer* buffer)
{
    DrawContext* ctx = static_cast<decltype(ctx)>(data);
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
    int32_t /*physical_width*/,
    int32_t /*physical_height*/,
    int32_t /*subpixel*/,
    const char */*make*/,
    const char */*model*/,
    int32_t /*transform*/)
{
    SwSplash::Self* self = static_cast<decltype(self)>(data);

    for (auto& oi : self->output_info)
    {
        if (wl_output == oi.wl_output)
        {
            oi.x = x;
            oi.y = y;
            return;
        }
    }
    self->output_info.push_back({wl_output, x, y, 0, 0});
}

void output_mode(void *data,
    struct wl_output *wl_output,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t /*refresh*/)
{
    if (!(WL_OUTPUT_MODE_CURRENT & flags))
        return;

    SwSplash::Self* self = static_cast<decltype(self)>(data);

    for (auto& oi : self->output_info)
    {
        if (wl_output == oi.wl_output)
        {
            oi.width = width;
            oi.height = height;
            return;
        }
    }
    self->output_info.push_back({wl_output, 0, 0, width, height});
}

void output_done(void* data, struct wl_output* wl_output)
{
    (void)data;
    (void)wl_output;
}

void output_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    (void)data;
    (void)wl_output;
    (void)factor;
}

wl_output_listener const output_listener = {
    &output_geometry,
    &output_mode,
    &output_done,
    &output_scale,
};
}

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
    struct wl_registry* registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, self.get());
    wl_display_roundtrip(display);

    wl_output_add_listener(self->output, &output_listener, self.get());
    wl_display_roundtrip(display);

    DrawContext* ctx = &self->ctx;

    for (auto const& oi : self->output_info)
    {
        ctx->width = std::max(ctx->width, oi.width);
        ctx->height = std::max(ctx->height, oi.height);
    }

    struct wl_shm_pool* shm_pool = make_shm_pool(self->shm, ctx->width * ctx->height * 4, &ctx->content_area);

    for (int i = 0; i < 4; ++i)
    {
        ctx->buffers[i].buffer =
            wl_shm_pool_create_buffer(shm_pool, 0, ctx->width, ctx->height, ctx->width*4, WL_SHM_FORMAT_ARGB8888);
        ctx->buffers[i].available = true;
        wl_buffer_add_listener(ctx->buffers[i].buffer, &buffer_listener, ctx);
    }

    ctx->display = display;
    ctx->surface = wl_compositor_create_surface(self->compositor);

    struct wl_shell_surface* window = wl_shell_get_shell_surface(self->shell, ctx->surface);
    wl_shell_surface_set_toplevel(window);

    struct wl_callback* first_frame = wl_display_sync(display);
    wl_callback_add_listener(first_frame, &frame_listener, ctx);

    auto const time_limit = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    do
    {
        wl_display_dispatch(display);

        for (auto& x : ctx->pattern)
            x =  3*x/4;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    while (std::chrono::steady_clock::now() < time_limit);
}
