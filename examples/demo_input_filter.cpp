/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/server.h"
#include "mir/report_exception.h"

#include "example_input_event_filter.h"
#include "mir/input/composite_event_filter.h"

#include <cstdlib>
#include <iostream>

namespace me = mir::examples;
namespace mi = mir::input;

namespace
{
struct PrintingEventFilter : public mi::EventFilter
{
    void print_motion_event(MirMotionEvent const& ev)
    {
        std::cout << "Motion Event time=" << ev.event_time
            << " pointer_count=" << ev.pointer_count << std::endl;

        for (size_t i = 0; i < ev.pointer_count; ++i)
        {
            std::cout << "  "
                << " id=" << ev.pointer_coordinates[i].id
                << " pos=(" << ev.pointer_coordinates[i].x << ", " << ev.pointer_coordinates[i].y << ")"
                << std::endl;
        }
        std::cout << "----------------" << std::endl << std::endl;
    }

    bool handle(MirEvent const& ev) override
    {
        // TODO: Enhance printing
        if (ev.type == mir_event_type_key)
        {
            std::cout << "Handling key event (time, scancode, keycode): " << ev.key.event_time << " " 
                << ev.key.scan_code << " " << ev.key.key_code << std::endl;
        }
        else if (ev.type == mir_event_type_motion)
        {
            print_motion_event(ev.motion);
        }
        return false;
    }
};
}

int main(int argc, char const* argv[])
try
{
    mir::Server server;

    // Set up Ctrl+Alt+BkSp => quit
    auto const quit_filter = std::make_shared<me::QuitFilter>([&]{ server.stop(); });
    server.add_init_callback([&] { server.the_composite_event_filter()->append(quit_filter); });

    // Set up a PrintingEventFilter
    auto const printing_filter = std::make_shared<PrintingEventFilter>();
    server.add_init_callback([&] { server.the_composite_event_filter()->prepend(printing_filter); });

    // Provide the command line and run the server
    server.set_command_line(argc, argv);
    server.apply_settings();
    server.run();
    return server.exited_normally() ? EXIT_SUCCESS : EXIT_FAILURE;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return EXIT_FAILURE;
}
