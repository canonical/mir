/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"
#include "mir_toolkit/mir_buffer.h"
#include <memory>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <fstream>

namespace
{
void remove_transparency(MirPixelFormat const& pf, MirGraphicsRegion const& region)
{
    auto const bytes_per_pixel = MIR_BYTES_PER_PIXEL(pf);
    auto const addr = region.vaddr;

    for (auto p = addr; p != addr + region.width * region.height * bytes_per_pixel; p += bytes_per_pixel)
        *(p+3) = '\xff';
}
}

int main(int argc, char *argv[])
try
{
    int arg = -1;
    static char const* socket_file = NULL;
    static char const* output_file = "screencap_output.raw";
    unsigned int width = 400;
    unsigned int height = 300;

    auto disp_id = 0;
    while ((arg = getopt (argc, argv, "m:f:d:s:h:")) != -1)
    {
        switch (arg)
        {
        case 'm':
            socket_file = optarg;
            break;
        case 'f':
            output_file = optarg;
            break;
        case 'd':
            disp_id = atoi(optarg);
            break;
        case 's':
            if (sscanf(optarg, "%ux%u", &width, &height) != 2 || width == 0 || height == 0)
            {
                std::cerr << "could not parse size " << optarg << '\n';
                return -1;
            }
            break;
        case 'h':
        default:
            std::cout << argv[0] << std::endl;
            std::cout << "Usage:\n";
            std::cout << "    -m <Mir server socket>\n";
            std::cout << "    -f file to output to\n";
            std::cout << "    -d output id to capture\n";
            std::cout << "    -s size of capture buffer (WxH)\n";
            std::cout << "    -h help dialog\n";
            return -1;
        }
    }

    int rc = 0;
    auto connection = mir_connect_sync(socket_file, "screencap_to_buffer");
    if (!mir_connection_is_valid(connection))
    {
        std::cerr << "could not connect to server\n";
        return -1;
    }

    auto display_config = mir_connection_create_display_configuration(connection);
    if (disp_id < 0 || disp_id > mir_display_config_get_num_outputs(display_config))
    {
        std::cerr << "invalid display id set\n";
        return -1;
    }

    auto output = mir_display_config_get_output(display_config, disp_id);
    auto mode = mir_output_get_current_mode(output);

    auto pf = mir_pixel_format_abgr_8888;
    MirRectangle rect {
        mir_output_get_position_x(output),
        mir_output_get_position_y(output),
        static_cast<unsigned int>(mir_output_mode_get_width(mode)),
        static_cast<unsigned int>(mir_output_mode_get_height(mode)) };
    mir_display_config_release(display_config);

    auto spec = mir_create_screencast_spec(connection);
    mir_screencast_spec_set_capture_region(spec, &rect);
    //TODO: the default screencast spec will capture a buffer when creating the screencast.
    //      Set to zero to avoid this, and when the old screencast-bufferstream method is removed,
    //      the initial capture will be removed. 
    mir_screencast_spec_set_number_of_buffers(spec, 0);

    auto screencast = mir_screencast_create_sync(spec);
    mir_screencast_spec_release(spec);

    auto buffer = mir_connection_allocate_buffer_sync(connection, width, height, pf);

    if (auto status = mir_screencast_capture_to_buffer_sync(screencast, buffer))
    {
        switch (status)
        {
            case mir_screencast_error_failure:
                std::cerr << "SCREENCAST FAILED: " << mir_screencast_get_error_message(screencast) << std::endl;
                rc = -1;
                break;
            default:
                break;
        }
    }  

    std::ofstream file(output_file);
    if (!rc && output)
    {
        MirBufferLayout layout;
        MirGraphicsRegion region;
        mir_buffer_map(buffer, &region, &layout);
        remove_transparency(pf, region);

        auto addr = region.vaddr;
        for (int i = 0; i < region.height; i++)
        {
            file.write(addr, region.width * MIR_BYTES_PER_PIXEL(pf));
            addr += region.stride;
        }

        mir_buffer_unmap(buffer);
    }

    mir_buffer_release(buffer);
    mir_screencast_release_sync(screencast);
    mir_connection_release(connection);
    return EXIT_SUCCESS;
}
catch(std::exception& e)
{
    std::cerr << "error : " << e.what() << std::endl;
    return EXIT_FAILURE;
}
