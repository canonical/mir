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
#include "wayland_app.h"
#include "wayland_surface.h"
#include "wayland_shm.h"
#include <wayland-client.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <mutex>
#include <map>

namespace
{
struct DrawContext
{
    int width = 400;
    int height = 400;

    void* content_area;
    struct wl_display* display;
    std::unique_ptr<WaylandSurface> surface;
    struct wl_callback* new_frame_signal;
    struct Buffer
    {
        wl_buffer* buffer = nullptr;
        bool available = false;
    } buffers[4];
    bool waiting_for_buffer;

    uint8_t pattern[4] = { 0x14, 0x48, 0xDD, 0xFF };
};

wl_buffer* find_free_buffer(DrawContext* ctx)
{
    for (auto& b: ctx->buffers)
    {
        if (b.available)
        {
            b.available = false;
            return b.buffer;
        }
    }
    return NULL;
}

void draw_new_stuff(DrawContext* ctx)
{
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

    ctx->surface->add_frame_callback([ctx]()
        {
            draw_new_stuff(ctx);
        });
    ctx->surface->attach_buffer(buffer, 1);
    ctx->surface->commit();
}

void update_free_buffers(void* data, struct wl_buffer* buffer)
{
    DrawContext* ctx = static_cast<decltype(ctx)>(data);
    for (auto& b: ctx->buffers)
    {
        if (b.buffer == buffer)
        {
            b.available = true;
        }
    }

    if (ctx->waiting_for_buffer)
    {
        WaylandCallback::create(wl_display_sync(ctx->display), [ctx]()
            {
                draw_new_stuff(ctx);
            });
    }

    ctx->waiting_for_buffer = false;
}

wl_buffer_listener const buffer_listener = {
    update_free_buffers
};
}

struct SwSplash::Self : SplashSession
{
    Self()
    {
    }

    bool show_splash=true;

    DrawContext ctx;

    std::mutex mutable mutex;
    std::weak_ptr<mir::scene::Session> session_;

    std::shared_ptr<mir::scene::Session> session() const override
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        return session_.lock();
    }

    void operator()(struct wl_display* display);
};

SwSplash::SwSplash() : self{std::make_shared<Self>()} {}

SwSplash::~SwSplash() = default;

void SwSplash::enable (bool show_splash_opt){
    self->show_splash = show_splash_opt;
}

void SwSplash::operator()(std::weak_ptr<mir::scene::Session> const& session)
{
    std::lock_guard<decltype(self->mutex)> lock{self->mutex};
    self->session_ = session;
}

SwSplash::operator std::shared_ptr<SplashSession>() const
{
    return self;
}

void SwSplash::Self::operator()(struct wl_display* display)
{
    if (!show_splash)
        return;

    WaylandApp app{display};

    ctx.display = display;
    ctx.surface = std::make_unique<WaylandSurface>(&app);
    ctx.surface->set_fullscreen(nullptr);
    ctx.surface->commit();
    app.roundtrip();

    ctx.width = ctx.surface->configured_size().width.as_int();
    ctx.height = ctx.surface->configured_size().height.as_int();

    struct wl_shm_pool* shm_pool = make_shm_pool(app.shm(), ctx.width * ctx.height * 4, &ctx.content_area);

    for (auto& b: ctx.buffers)
    {
        b.buffer = wl_shm_pool_create_buffer(shm_pool, 0, ctx.width, ctx.height, ctx.width*4, WL_SHM_FORMAT_ARGB8888);
        b.available = true;
        wl_buffer_add_listener(b.buffer, &buffer_listener, &ctx);
    }

    wl_shm_pool_destroy(shm_pool);

    WaylandCallback::create(wl_display_sync(display), [this]()
        {
            draw_new_stuff(&ctx);
        });

    auto const time_limit = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    //this is the splash screen
    do
    {
        wl_display_dispatch(display);

        for (auto& x : ctx.pattern)
        {
            x =  3*x/4;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    while (std::chrono::steady_clock::now() < time_limit);

    for (auto const& b: ctx.buffers)
    {
        if (b.buffer)
            wl_buffer_destroy(b.buffer);
    }

    ctx.surface.reset();
}

void SwSplash::operator()(struct wl_display* display)
{
    (*self)(display);
}
