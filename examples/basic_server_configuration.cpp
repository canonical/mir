/*
 * Copyright © 2012, 2014 Canonical Ltd.
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

#include "basic_server_configuration.h"
#include "mir/options/default_configuration.h"

#include "mir/abnormal_exit.h"
#include "mir/frontend/connector.h"
#include "mir/options/option.h"

#include <cstdlib>

namespace
{
char const* const launch_child_opt = "launch-client";
}

namespace mir
{
namespace examples
{

BasicServerConfiguration::BasicServerConfiguration(int argc, char const** argv) :
    ServerConfiguration([argc, argv]
    {
        auto result = std::make_shared<options::DefaultConfiguration>(argc, argv);

        namespace po = boost::program_options;

        result->add_options()
            (launch_child_opt, po::value<std::string>(), "system() command to launch client");

        return result;
    }())
{
}

void BasicServerConfiguration::launch_client()
{
    if (the_options()->is_set(launch_child_opt))
    {
        char buffer[128] = {0};
        sprintf(buffer, "fd://%d", the_connector()->client_socket_fd());
        setenv("MIR_SOCKET", buffer, 1);
        auto ignore = std::system((the_options()->get<std::string>(launch_child_opt) + "&").c_str());
        (void)ignore;
    }
}

}
}
