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
#include "../server_configuration.h"

#include "mir/report_exception.h"

#include <iostream>

namespace me = mir::examples;

///\page demo_inprocess_egl demo_inprocess_egl.cpp: A simple use of egl in process
///\section main main
/// The main() function uses a default configuration for Mir and sets up an InprocessEGLClient
/// that accesses the graphics platform and surface factory.
/// \snippet demo_inprocess_egl.cpp main_tag
/// This InprocessEGLClient sets up a single surface
/// \snippet inprocess_egl_client.cpp setup_tag
/// And loops updating the surface
/// \snippet inprocess_egl_client.cpp loop_tag

int main(int argc, char const* argv[])
try
{
    ///\internal [main_tag]
    me::ServerConfiguration config(argc, argv);

    std::shared_ptr<me::InprocessEGLClient> client;

    mir::run_mir(config, [&config, &client](mir::DisplayServer&)
    {
        client = std::make_shared<me::InprocessEGLClient>(
              config.the_graphics_platform(),
              config.the_frontend_shell(),
              config.the_focus_controller());
    });
    ///\internal [main_tag]

    return 0;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return 1;
}
