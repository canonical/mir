/*
 * Copyright © 2015 Canonical Ltd.
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

#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "mir_toolkit/mir_client_library.h"

#include "client_helpers.h"

namespace po = boost::program_options;

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
          buffer{region}
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
    return pixel_iterator{region};
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

    mir_buffer_stream_swap_buffers_sync(stream);
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

    me::NormalSurface surface{connection, 200, 200, true, false};
    MirBufferStream* surface_stream = mir_surface_get_buffer_stream(surface);
    me::BufferStream top{connection, 100, 100, true, false};
    me::BufferStream bottom{connection, 50, 50, true, false};

    fill_stream_with(surface_stream, 255, 0, 0, 128);
    fill_stream_with(top, 0, 255, 0, 128);
    fill_stream_with(bottom, 0, 0, 255, 128);

    std::array<MirBufferStreamInfo, 3> arrangement;

    arrangement[0].displacement_x = 0;
    arrangement[0].displacement_y = 0;
    arrangement[0].stream = surface_stream;

    arrangement[1].displacement_x = 50;
    arrangement[1].displacement_y = 50;
    arrangement[1].stream = bottom;

    arrangement[2].displacement_x = -40;
    arrangement[2].displacement_y = -10;
    arrangement[2].stream = top;

    int top_dx{1}, top_dy{2};
    int bottom_dx{2}, bottom_dy{-1};

    auto spec = mir_connection_create_spec_for_changes(connection);

    int baseColour = 255, dbase = 1;
    int topColour = 255, dtop = 1;
    int bottomColour = 255, dbottom = 1;

    while (1)
    {
        bounce_position(arrangement[1].displacement_x, bottom_dx, -100, 300);
        bounce_position(arrangement[1].displacement_y, bottom_dy, -100, 300);
        bounce_position(arrangement[2].displacement_x, top_dx, -100, 300);
        bounce_position(arrangement[2].displacement_y, top_dy, -100, 300);

        mir_surface_spec_set_streams(spec, arrangement.data(), arrangement.size());
        mir_surface_apply_spec(surface, spec);

        bounce_position(baseColour, dbase, 128, 255);
        bounce_position(topColour, dtop, 200, 255);
        bounce_position(bottomColour, dbottom, 100, 255);


        fill_stream_with(surface_stream, baseColour, 0, 0, 128);
        fill_stream_with(bottom, 0, 0, bottomColour, 128);
        fill_stream_with(top, 0, topColour, 0, 128);

        mir_buffer_stream_swap_buffers_sync(surface_stream);
        mir_buffer_stream_swap_buffers_sync(bottom);
        mir_buffer_stream_swap_buffers_sync(top);
    }
    mir_surface_spec_release(spec);

    return 0;
}
