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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir/raii.h"
#include <thread>
#include <memory>
#include <chrono>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <mutex>
#include <fstream>
#include <condition_variable>

int main(int argc, char *argv[])
try
{
    int arg = -1;
    static char const *socket_file = NULL;
    static char const *output_file = "screencap_output.raw";
    auto disp_id = 0;
    while ((arg = getopt (argc, argv, "m:f:d:h:")) != -1)
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
        case 'h':
        default:
            puts(argv[0]);
            printf("Usage:\n");
            printf("    -m <Mir server socket>\n");
            printf("    -f file to output to\n");
            printf("    -d output id to capture\n");
            printf("    -h help dialog\n");
            return -1;
        }
    }


    int rc = 0;
    auto connection = mir_connect_sync(socket_file, "screencap_to_buffer");

    auto const display_config = mir::raii::deleter_for(
        mir_connection_create_display_configuration(connection),
        &mir_display_config_release);

    if (disp_id < 0 || disp_id > mir_display_config_get_num_outputs(display_config.get()))
    {
        std::cerr << "invalid display id set\n";
        return -1;
    }

    auto output = mir_display_config_get_output(display_config.get(), disp_id);
    auto mode = mir_output_get_current_mode(output);

    auto pf = mir_pixel_format_abgr_8888;
    unsigned int width = 400;
    unsigned int height = 300;
    MirRectangle rect {
        mir_output_get_position_x(output),
        mir_output_get_position_y(output),
        static_cast<unsigned int>(mir_output_mode_get_width(mode)),
        static_cast<unsigned int>(mir_output_mode_get_height(mode)) };
    auto spec = mir_create_screencast_spec(connection);
    mir_screencast_spec_set_width(spec, width);
    mir_screencast_spec_set_height(spec, height);
    mir_screencast_spec_set_capture_region(spec, &rect);
    mir_screencast_spec_set_mirror_mode(spec, mir_mirror_mode_none);

    //TODO: the default screencast spec will capture a buffer when creating the screencast.
    //      Set to zero to avoid this, and when the old screencast-bufferstream method is removed,
    //      the initial capture will be removed. 
    mir_screencast_spec_set_number_of_buffers(spec, 0);

    mir_screencast_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto screencast = mir_screencast_create_sync(spec);
    mir_screencast_spec_release(spec);

    auto buffer = mir_connection_allocate_buffer_sync(connection, width, height, pf);

    struct Capture
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
    } cap;

    auto error = mir_screencast_capture_to_buffer_sync(screencast, buffer);
    if (error && mir_error_get_domain(error) == mir_error_domain_screencast)
    {
        auto code = mir_error_get_code(error);
        switch (code)
        {
            case mir_screencast_error_unsupported:
                std::cerr << "screencast unsupported" << std::endl;
                rc = -1;
                break;
            case mir_screencast_error_failure:
                std::cerr << "screencast failed" << std::endl;
                rc = -1;
                break;
            case mir_screencast_performance_warning:
                std::cout << "screencast performance warning" << std::endl;
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
        auto addr = region.vaddr + (region.height - 1)*region.stride;
        for (int i = 0; i < region.height; i++)
        {
            file.write(addr, region.width*4);
            addr -= region.stride;
        }
        mir_buffer_unmap(buffer);
    }

    printf("hram?\n");
//    mir_screencast_release_sync(screencast);
//    mir_connection_release(connection);
    return 0;
}
catch(std::exception& e)
{
    std::cerr << "error : " << e.what() << std::endl;
    return 1;
}
