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

#include "mir/display_server.h"
#include "mir/server_configuration.h"
#include "mir/thread/all.h"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/config.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

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

int main(int argc, char* argv[])
try
{
    namespace po = boost::program_options;

    po::options_description desc("Options");
    po::variables_map options;

    std::string socket_file{"/tmp/mir_socket"};

    try
    {
        desc.add_options()
            ("file,f", po::value<std::string>(), "<socket filename>")
            ("help,h", "this help text");

        po::store(po::parse_command_line(argc, argv, desc), options);
        po::notify(options);
    }
    catch (po::error const& error)
    {
        std::cerr << "ERROR: " << error.what() << std::endl;
        std::cerr << desc << "\n";
        return 1;
    }

    if (options.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }
    else if (options.count("file"))
    {
        socket_file = options["file"].as<std::string>();
    }

    run_mir(socket_file);
    return 0;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
}
