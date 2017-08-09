/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *         Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <signal.h>
#include <sys/signalfd.h>
#include <poll.h>

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/rs/mir_render_surface.h"

#include "client_helpers.h"

namespace me = mir::examples;

class Pixel
{
public:
    Pixel(void* addr, MirPixelFormat format)
        : addr{addr},
          format{format}
    {
    }

    void write(int r, int g, int b, int a)
    {
        switch (format)
        {
        case mir_pixel_format_abgr_8888:
            *((uint32_t*) addr) =
                (uint32_t) a << 24 |
                (uint32_t) b << 16 |
                (uint32_t) g << 8 |
                (uint32_t) r;
            break;
        case mir_pixel_format_xbgr_8888:
            *((uint32_t*) addr) =
                /* Not filling in the X byte is correct but buggy (LP: #1423462) */
                (uint32_t) b << 16 |
                (uint32_t) g << 8 |
                (uint32_t) r;
            break;
        case mir_pixel_format_argb_8888:
            *((uint32_t*) addr) =
                (uint32_t) a << 24 |
                (uint32_t) r << 16 |
                (uint32_t) g << 8 |
                (uint32_t) b;
            break;
        case mir_pixel_format_xrgb_8888:
            *((uint32_t*) addr) =
                /* Not filling in the X byte is correct but buggy (LP: #1423462) */
                (uint32_t) r << 16 |
                (uint32_t) g << 8 |
                (uint32_t) b;
            break;
        case mir_pixel_format_rgb_888:
            *((uint8_t*) addr) = r;
            *((uint8_t*) addr + 1) = g;
            *((uint8_t*) addr + 2) = b;
            break;
        case mir_pixel_format_bgr_888:
            *((uint8_t*) addr) = b;
            *((uint8_t*) addr + 1) = g;
            *((uint8_t*) addr + 2) = r;
            break;
        default:
            throw std::runtime_error{"Pixel format unsupported by Pixel::write!"};
        }
    }

public:
    void* const addr;
    MirPixelFormat const format;
};

class pixel_iterator : std::iterator<std::random_access_iterator_tag, Pixel>
{
public:
    pixel_iterator(MirGraphicsRegion const& region, int x, int y)
        : x{x},
          y{y},
          buffer(region)
    {
    }

    pixel_iterator(MirGraphicsRegion const& region)
       : pixel_iterator(region, 0, 0)
    {
    }


    pixel_iterator& operator++()
    {
        x++;
        if (x == buffer.width)
        {
            x = 0;
            y++;
        }
        return *this;
    }
    pixel_iterator operator++(int)
    {
        auto old = *this;
        ++(*this);
        return old;
    }

    Pixel operator*() const
    {
        return Pixel{
            buffer.vaddr + (x * MIR_BYTES_PER_PIXEL(buffer.pixel_format))
                         + (y * buffer.stride), buffer.pixel_format};
    }

    bool operator==(pixel_iterator const& rhs)
    {
        return rhs.buffer.vaddr == buffer.vaddr &&
               rhs.x == x &&
               rhs.y == y;
    }

    bool operator!=(pixel_iterator const& rhs)
    {
        return !(*this == rhs);
    }

private:
    int x, y;
    MirGraphicsRegion const buffer;
};

pixel_iterator begin(MirGraphicsRegion const& region)
{
    return pixel_iterator(region);
}

pixel_iterator end(MirGraphicsRegion const& region)
{
    return pixel_iterator{region, 0, region.height};
}

void fill_stream_with(MirBufferStream* stream, int r, int g, int b, int a)
{
    MirGraphicsRegion buffer;
    mir_buffer_stream_get_graphics_region(stream, &buffer);

    for (auto pixel : buffer)
    {
        pixel.write(r, g, b, a);
    }
}

void bounce_position(int& position, int& delta, int min, int max)
{
    if (position <= min || position >= max)
    {
        delta = -delta;
    }
    position += delta;
}

int main(int /*argc*/, char* /*argv*/[])
{
    char const* socket = nullptr;
    int const width = 200;
    int const height = 200;
    int baseColour = 255, dbase = 1;
    unsigned int nformats{0};
    MirPixelFormat pixel_format;

    me::Connection connection{socket, "MirRenderSurface example"};

    auto render_surface = mir_connection_create_render_surface_sync(connection, width, height);
    if (!mir_render_surface_is_valid(render_surface))
        throw std::runtime_error(
                  std::string(mir_render_surface_get_error_message(render_surface)));

    auto spec = mir_create_normal_window_spec(connection, width, height);

    mir_window_spec_set_name(spec, "Stream");

    mir_window_spec_add_render_surface(spec, render_surface, width, height, 0, 0);

    mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &nformats);
    if (nformats == 0)
        throw std::runtime_error("no pixel formats for buffer stream");
    printf("Software Driver selected pixel format %d\n", pixel_format);
    auto buffer_stream = mir_render_surface_get_buffer_stream(
        render_surface, width, height, pixel_format);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    fill_stream_with(buffer_stream, 255, 0, 0, 128);
    mir_buffer_stream_swap_buffers_sync(buffer_stream);

    sigset_t halt_signals;
    sigemptyset(&halt_signals);
    sigaddset(&halt_signals, SIGTERM);
    sigaddset(&halt_signals, SIGQUIT);
    sigaddset(&halt_signals, SIGINT);

    sigprocmask(SIG_BLOCK, &halt_signals, nullptr);
    int const signal_watch{signalfd(-1, &halt_signals, SFD_CLOEXEC)};

    pollfd signal_poll{
        signal_watch,
        POLLIN | POLLERR,
        0
    };

    while (poll(&signal_poll, 1, 0) <= 0)
    {
        bounce_position(baseColour, dbase, 128, 255);

        fill_stream_with(buffer_stream, baseColour, 0, 0, 128);

        mir_buffer_stream_swap_buffers_sync(buffer_stream);
    }

    mir_render_surface_release(render_surface);
    mir_window_release_sync(window);
    close(signal_watch);

    return 0;
}
