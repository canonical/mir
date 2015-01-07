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
void print_key_event(MirInputEvent const* ev)
{
    auto event_time = mir_input_event_get_event_time(ev);
    auto kev = mir_input_event_get_key_input_event(ev);
    auto scan_code = mir_key_input_event_get_scan_code(kev);
    auto key_code = mir_key_input_event_get_key_code(kev);

    std::cout << "Handling key event (time, scancode, keycode): " << event_time << " " <<
              scan_code << " " << key_code << std::endl;
}

void print_touch_event(MirInputEvent const* ev)
{
    auto event_time = mir_input_event_get_event_time(ev);
    auto tev = mir_input_event_get_touch_input_event(ev);
    auto tc = mir_touch_input_event_get_touch_count(tev);

    std::cout << "Handline touch event time=" << event_time
              << " touch_count=" << tc << std::endl;
    for (unsigned i = 0; i < tc; i++)
    {
        auto id = mir_touch_input_event_get_touch_id(tev, i);
        auto px = mir_touch_input_event_get_touch_axis_value(tev, i, 
            mir_touch_input_axis_x);
        auto py = mir_touch_input_event_get_touch_axis_value(tev, i, 
            mir_touch_input_axis_y);

        std::cout << "  "
            << " id=" << id
            << " pos=(" << px << ", " << py << ")"
            << std::endl;
    }
    std::cout << "----------------" << std::endl << std::endl;
}

struct PrintingEventFilter : public mi::EventFilter
{
    bool handle(MirEvent const& ev) override
    {
        if (mir_event_get_type(&ev) != mir_event_type_input)
            return false;
        auto input_event = mir_event_get_input_event(&ev);

        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_key:
            print_key_event(input_event);
            break;
        case mir_input_event_type_touch:
            print_touch_event(input_event);
            break;
        default:
            abort();
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

