/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/options/program_option.h"
#include "mir/display_server.h"
#include "mir/server_configuration.h"
#include "mir/thread/all.h"

#include <csignal>
#include <iostream>
#include <stdexcept>

namespace
{
// TODO: Get rid of the volatile-hack here and replace it with
// some sane atomic-pointer once we have left GCC 4.4 behind.
mir::DisplayServer* volatile signal_display_server;
}

namespace mir
{
extern "C"
{
void signal_terminate (int )
{
    while (!signal_display_server)
        std::this_thread::yield();

    signal_display_server->stop();
}
}
}

namespace
{
void run_mir(std::string const& socket_file)
{
    signal(SIGINT, mir::signal_terminate);
    signal(SIGTERM, mir::signal_terminate);

    mir::DefaultServerConfiguration config(socket_file);
    mir::DisplayServer server(config);

    signal_display_server = &server;

    server.start();
}
}

int main(int argc, char const* argv[])
try
{
    namespace po = boost::program_options;

    mir::options::ProgramOption options;

    po::options_description desc("Options");

    try
    {
        desc.add_options()
            ("file,f", po::value<std::string>(), "<socket filename>")
            ("help,h", "this help text");

        options.parse_arguments(desc, argc, argv);
    }
    catch (po::error const& error)
    {
        std::cerr << "ERROR: " << error.what() << std::endl;
        std::cerr << desc << "\n";
        return 1;
    }

    if (options.is_set("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }

    run_mir(options.get("file", "/tmp/mir_socket"));
    return 0;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
}
