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

#include "inprocess_egl_client.h"

#include "mir/run_mir.h"
#include "mir/default_server_configuration.h"

#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

namespace me = mir::examples;

int main(int argc, char const* argv[])
try
{
    mir::DefaultServerConfiguration config(argc, argv);

    std::shared_ptr<me::InprocessEGLClient> client;
    mir::run_mir(config, [&config, &client](mir::DisplayServer&)
    {
        client = std::make_shared<me::InprocessEGLClient>(
              config.the_graphics_platform(),
              config.the_shell_surface_factory());
    });

    return 0;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << boost::diagnostic_information(error) << std::endl;
    return 1;
}
