/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/run_mir.h"
#include "mir/abnormal_exit.h"
#include "mir/input/composite_event_filter.h"
#include "server_configuration.h"

#include <boost/exception/diagnostic_information.hpp>

#include <iostream>

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

struct DemoServerConfiguration : public mir::examples::ServerConfiguration
{
    DemoServerConfiguration(int argc, char const* argv[])
      : ServerConfiguration(argc, argv),
        event_filter(std::make_shared<PrintingEventFilter>())
    {
    }

    std::shared_ptr<mi::CompositeEventFilter> the_composite_event_filter() override
    {
        auto composite_filter = ServerConfiguration::the_composite_event_filter();
        composite_filter->prepend(event_filter);
        return composite_filter;
    }

    std::shared_ptr<PrintingEventFilter> const event_filter;
};

}


#include <std/MirLog.h>
void my_write_to_log(int /*prio*/, char const* buffer)
{
    printf("%s\n", buffer);
}

int main(int argc, char const* argv[])
try
{
    DemoServerConfiguration config(argc, argv);
    mir::write_to_log = my_write_to_log;

    mir::run_mir(config, [](mir::DisplayServer&) {/* empty init */});
    return 0;
}
catch (mir::AbnormalExit const& error)
{
    std::cerr << error.what() << std::endl;
    return 1;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << boost::diagnostic_information(error) << std::endl;
    return 1;
}
