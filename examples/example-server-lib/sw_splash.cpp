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
 */

#include "sw_splash.h"
#include "wayland_app.h"
#include "wayland_surface.h"
#include "wayland_shm.h"
#include <wayland-client.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <mutex>

using namespace mir::geometry;

namespace
{
struct DrawContext
{
    struct wl_display* display;
    std::unique_ptr<WaylandShm> shm;
    std::unique_ptr<WaylandSurface> surface;

    uint8_t pattern[4] = { 0x14, 0x48, 0xDD, 0xFF };
};

void draw_new_stuff(DrawContext* ctx)
{
    Size const size{ctx->surface->configured_size()};
    auto const width = size.width.as_int();
    auto const height = size.height.as_int();
    auto const stride = width * 4;

    auto const buffer = ctx->shm->get_buffer(size, Stride{stride});
    char* row = static_cast<decltype(row)>(buffer->data());

    for (int j = 0; j < height; j++)
    {
        uint32_t* pixel = (uint32_t*)row;

        for (int i = 0; i < width; i++)
            memcpy(pixel + i, ctx->pattern, sizeof pixel[i]);

        row += stride;
    }

    ctx->surface->add_frame_callback([ctx]()
        {
            draw_new_stuff(ctx);
        });
    ctx->surface->attach_buffer(buffer->use(), 1);
    ctx->surface->commit();
}
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
        std::lock_guard lock{mutex};
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
    std::lock_guard lock{self->mutex};
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
    ctx.shm = std::make_unique<WaylandShm>(app.shm());
    ctx.surface = std::make_unique<WaylandSurface>(&app);
    ctx.surface->set_fullscreen(nullptr);
    ctx.surface->commit();
    app.roundtrip();

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

    ctx.surface.reset();
    ctx.shm.reset();
}

void SwSplash::operator()(struct wl_display* display)
{
    (*self)(display);
}
