/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_input_filter.h"

#include "mir/server.h"

#include "mir/input/composite_event_filter.h"
#include "mir/options/option.h"

#include <iostream>

namespace me = mir::examples;
namespace mi = mir::input;

///\example server_example_input_filter.cpp
/// Demonstrate adding a custom input filter to a server
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

auto me::make_printing_input_filter_for(mir::Server& server)
-> std::shared_ptr<mi::EventFilter>
{
    static const char* const print_input_events = "print-input-events";
    static const char* const print_input_events_descr = "List input events on std::cout";

    server.add_configuration_option(print_input_events, print_input_events_descr, mir::OptionType::null);

    auto const printing_filter = std::make_shared<PrintingEventFilter>();

    server.add_init_callback([printing_filter, &server]
        {
            const auto options = server.get_options();
            if (options->is_set(print_input_events))
                server.the_composite_event_filter()->prepend(printing_filter);
        });

    return printing_filter;
}

