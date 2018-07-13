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

#include <mir/client/display_config.h>
#include <mir/client/surface.h>
#include <mir/client/window.h>
#include <mir/client/window_spec.h>

#include <mir_toolkit/mir_buffer_stream.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <mutex>

namespace
{
MirPixelFormat find_8888_format(MirConnection* connection)
{
    unsigned int const num_formats = 32;
    MirPixelFormat pixel_formats[num_formats];
    unsigned int valid_formats;
    mir_connection_get_available_surface_formats(connection, pixel_formats, num_formats, &valid_formats);

    for (unsigned int i = 0; i < num_formats; ++i)
    {
        MirPixelFormat cur_pf = pixel_formats[i];
        if (cur_pf == mir_pixel_format_abgr_8888 ||
            cur_pf == mir_pixel_format_argb_8888)
        {
            return cur_pf;
        }
    }

    for (unsigned int i = 0; i < num_formats; ++i)
    {
        MirPixelFormat cur_pf = pixel_formats[i];
        if (cur_pf == mir_pixel_format_xbgr_8888 ||
            cur_pf == mir_pixel_format_xrgb_8888)
        {
            return cur_pf;
        }
    }

    return *pixel_formats;
}

auto create_window(MirConnection* connection, mir::client::Surface const& surface) -> mir::client::Window
{
    int id = 0;
    int width = 0;
    int height = 0;

    mir::client::DisplayConfig{connection}.for_each_output([&](MirOutput const* output)
        {
            if (mir_output_get_connection_state(output) == mir_output_connection_state_connected &&
                mir_output_is_enabled(output))
            {
                id = mir_output_get_id(output);

                MirOutputMode const* mode = mir_output_get_current_mode(output);
                width = mir_output_mode_get_width(mode);
                height = mir_output_mode_get_height(mode);
            }
        });

    return mir::client::WindowSpec::for_normal_window(connection, width, height)
        .set_name("splash")
        .set_fullscreen_on_output(id)
        .add_surface(surface, width, height, 0, 0)
        .create_window();
}

void render_pattern(MirGraphicsRegion *region, uint8_t pattern[])
{
    char *row = region->vaddr;

    for (int j = 0; j < region->height; j++)
    {
        uint32_t *pixel = (uint32_t*)row;

        for (int i = 0; i < region->width; i++)
            memcpy(pixel+i, pattern, sizeof pixel[i]);

        row += region->stride;
    }
}
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

void SwSplash::operator()(MirConnection* connection)
{
    MirPixelFormat pixel_format = find_8888_format(connection);

    uint8_t pattern[4] = { 0x14, 0x48, 0xDD, 0xFF };

    switch(pixel_format)
    {
    case mir_pixel_format_abgr_8888:
    case mir_pixel_format_xbgr_8888:
        std::swap(pattern[2],pattern[0]);
        break;

    case mir_pixel_format_argb_8888:
    case mir_pixel_format_xrgb_8888:
        break;

    default:
        return;
    };


    mir::client::Surface surface{mir_connection_create_render_surface_sync(connection, 42, 42)};
    MirBufferStream* buffer_stream = mir_render_surface_get_buffer_stream(surface, 42, 42, pixel_format);

    auto const window = create_window(connection, surface);

    MirGraphicsRegion graphics_region;

    auto const time_limit = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    do
    {
        mir_buffer_stream_get_graphics_region(buffer_stream, &graphics_region);

        render_pattern(&graphics_region, pattern);
        mir_buffer_stream_swap_buffers_sync(buffer_stream);

        for (auto& x : pattern)
            x =  3*x/4;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    while (std::chrono::steady_clock::now() < time_limit);
}
