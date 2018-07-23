/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include <boost/program_options.hpp>
#include <iostream>
#include <thread>
#include <signal.h>
#include <sys/signalfd.h>
#include <poll.h>

#include "mir_toolkit/mir_client_library.h"

#include "client_helpers.h"

#include <memory>

namespace po = boost::program_options;
namespace me = mir::examples;

namespace 
{
class MyBufferStream
{
public:
    MyBufferStream(
        me::Connection& connection,
        int width,
        int height,
        int displacement_x,
        int displacement_y);

    operator MirBufferStream*() const { return bs; }
    operator MirRenderSurface*() const;

    int displacement_x{0};
    int displacement_y{0};

    MyBufferStream& operator=(MyBufferStream &&) = default;
    MyBufferStream(MyBufferStream &&) = default;

private:

    MirBufferStream* get_stream(MirConnection* connection, int width, int height) const;

    std::unique_ptr<MirRenderSurface, decltype(&mir_render_surface_release)> stream;
    MirBufferStream* bs;

    MyBufferStream(MyBufferStream const&) = delete;
    MyBufferStream& operator=(MyBufferStream const&) = delete;
};

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
}


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
            buffer.vaddr + (x * MIR_BYTES_PER_PIXEL(buffer.pixel_format))  + (y * buffer.stride), buffer.pixel_format};
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


int main(int argc, char* argv[])
{
    po::options_description desc("Mir multi-bufferstream example:");
    desc.add_options()
        ("help", "this help message")
        ("socket", po::value<std::string>(), "Server socket to connect to");

    po::variables_map options;
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);

    if (options.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    char const* socket = nullptr;
    if (options.count("socket"))
    {
        socket = options["socket"].as<std::string>().c_str();
    }

    me::Connection connection{socket, "Multiple MirBufferStream example"};

    std::vector<MyBufferStream> stream;

    stream.emplace_back(connection, 200, 200, 0, 0);
    fill_stream_with(stream[0], 255, 0, 0, 128);
    mir_buffer_stream_swap_buffers_sync(stream[0]);

    stream.emplace_back(connection, 50, 50, 50, 50);
    fill_stream_with(stream[1], 0, 0, 255, 128);
    mir_buffer_stream_swap_buffers_sync(stream[1]);

    int topSize = 100, dTopSize = 2;
    stream.emplace_back(connection, topSize, topSize, -40, -10);
    fill_stream_with(stream[2], 0, 255, 0, 128);
    mir_buffer_stream_swap_buffers_sync(stream[2]);

    int top_dx{1}, top_dy{2};
    int bottom_dx{2}, bottom_dy{-1};

    int baseColour = 255, dbase = 1;
    int topColour = 255, dtop = 1;
    int bottomColour = 255, dbottom = 1;

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

    me::NormalWindow window{connection, 200, 200, stream[0]};

    while (poll(&signal_poll, 1, 0) <= 0)
    {
        bounce_position(stream[1].displacement_x, bottom_dx, -100, 300);
        bounce_position(stream[1].displacement_y, bottom_dy, -100, 300);
        bounce_position(stream[2].displacement_x, top_dx, -100, 300);
        bounce_position(stream[2].displacement_y, top_dy, -100, 300);

        bounce_position(baseColour, dbase, 128, 255);
        bounce_position(topColour, dtop, 200, 255);
        bounce_position(bottomColour, dbottom, 100, 255);

        bounce_position(topSize, dTopSize, 70, 120);

        stream[2] = MyBufferStream{connection, topSize, topSize, stream[2].displacement_x, stream[2].displacement_y};

        fill_stream_with(stream[0], baseColour, 0, 0, 128);
        fill_stream_with(stream[1], 0, 0, bottomColour, 128);
        fill_stream_with(stream[2], 0, topColour, 0, 128);

        auto spec = mir_create_window_spec(connection);
        for (auto& s : stream)
        {
            int width{0};
            int height{0};
            mir_render_surface_get_size(s, &width, &height);

            mir_buffer_stream_swap_buffers_sync(s);
            mir_window_spec_add_render_surface(spec, s, width, height, s.displacement_x, s.displacement_y);
        }
        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
    }
    close(signal_watch);

    std::cout << "Quitting; have a nice day." << std::endl;
    return 0;
}

MyBufferStream::MyBufferStream(
    me::Connection& connection,
    int width,
    int height,
    int displacement_x,
    int displacement_y)
    : displacement_x{displacement_x},
      displacement_y{displacement_y},
      stream{mir_connection_create_render_surface_sync(connection, width, height), &mir_render_surface_release},
      bs{get_stream(connection, width, height)}
{
}

MyBufferStream::operator MirRenderSurface*() const
{
    return stream.get();
}

MirBufferStream* MyBufferStream::get_stream(MirConnection* connection, int width, int height) const
{
    MirPixelFormat selected_format;
    unsigned int valid_formats{0};
    MirPixelFormat pixel_formats[mir_pixel_formats];
    mir_connection_get_available_surface_formats(connection, pixel_formats, mir_pixel_formats, &valid_formats);
    if (valid_formats == 0)
        throw std::runtime_error("no pixel formats for buffer stream");
    selected_format = pixel_formats[0];

    return mir_render_surface_get_buffer_stream(stream.get(), width, height, selected_format);
}
