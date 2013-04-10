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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/run_mir.h"
#include "mir/default_server_configuration.h"
#include "mir/abnormal_exit.h"
#include "mir/input/event_filter.h"

#include <boost/exception/diagnostic_information.hpp>

#include <iostream>

namespace mi = mir::input;

namespace
{

struct PrintingEventFilter : public mi::EventFilter
{
    bool handles(MirEvent const& ev) override
    {
        // TODO: Enhance printing
        (void) ev;
        std::cout << "Handling event" << std::endl;

        return false;
    }
};

struct DemoServerConfiguration : public mir::DefaultServerConfiguration
{
    DemoServerConfiguration(int argc, char const* argv[])
      : DefaultServerConfiguration(argc, argv),
        event_filter(std::make_shared<PrintingEventFilter>())
    {
    }
    
    std::initializer_list<std::shared_ptr<mi::EventFilter> const> the_event_filters() override
    {
        return { event_filter };
    }

    std::shared_ptr<PrintingEventFilter> event_filter;
};

}

int main(int argc, char const* argv[])
try
{
    DemoServerConfiguration config(argc, argv);

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
