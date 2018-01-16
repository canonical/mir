/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_configuration.h"

#include <boost/filesystem.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

using namespace boost::program_options;
using namespace std::chrono;

namespace
{
auto canonicalize(std::string search_path) -> std::string
{
    using namespace boost::filesystem;

    std::string result;

    for (auto i = begin(search_path); i != end(search_path); )
    {
        auto const j = find(i, end(search_path), ':');

        auto element = canonical(path{i, j});

        if (result.size()) result += ':';
        result += element.string();

        if ((i = j) != end(search_path)) ++i;
    }

    return result;
}
}

auto main(int argc, char* argv[]) -> int
try
{
    auto const help = "help";
    auto const socket = "socket";
    auto const wait = "wait";

    options_description desc(std::string{argv[0]} + " [options] <program>" );

    desc.add_options()
        (socket, value<std::string>(), "Server socket to connect to")
        (wait, value<unsigned>()->default_value(0u), "Max wait for socket creation (seconds)")
        (help, "this help message");

    variables_map options;
    auto const parsed_command_line = command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    store(parsed_command_line, options);
    notify(options);

    auto unparsed_tokens = collect_unrecognized(parsed_command_line.options, include_positional);

    if (options.count(help) || unparsed_tokens.size() < 1)
    {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    unsetenv("DISPLAY");                                // Discourage toolkits from using X11
    setenv("GDK_BACKEND", "mir", true);                 // configure GTK to use Mir
    setenv("QT_QPA_PLATFORM", "wayland", true);         // configure Qt to use Mir
    unsetenv("QT_QPA_PLATFORMTHEME");                   // Discourage Qt from unsupported theme
    setenv("SDL_VIDEODRIVER", "wayland", true);         // configure SDL to use Wayland

    if (auto const client_platform = getenv("MIR_CLIENT_PLATFORM_PATH"))
        setenv("MIR_CLIENT_PLATFORM_PATH", canonicalize(client_platform).c_str(), true);

    if (auto const library_path = getenv("LD_LIBRARY_PATH"))
        setenv("LD_LIBRARY_PATH", canonicalize(library_path).c_str(), true);

    std::string mir_socket{mir::default_server_socket};

    if (options.count(socket))
    {
        mir_socket = options[socket].as<std::string>();
        setenv("MIR_SOCKET", mir_socket.c_str(), true);
    }

    auto const time_limit = steady_clock::now() + seconds(options[wait].as<unsigned>());

    while (steady_clock::now() < time_limit)
    {
        if (mir_socket.find("fd://") == 0)
            throw std::runtime_error("Can't wait for file descriptor: " + mir_socket);

        boost::filesystem::path endpoint{mir_socket};

        if (exists(endpoint))
            break;

        std::this_thread::sleep_for(milliseconds(10));
    }

    // Special treatment for gnome-terminal (it's odd)
    if (unparsed_tokens[0] == "gnome-terminal")
    {
        if (std::find(begin(unparsed_tokens), end(unparsed_tokens), "--app-id") == end(unparsed_tokens))
        {
            unparsed_tokens.emplace_back("--app-id");
            unparsed_tokens.emplace_back("com.canonical.mir.Terminal");
        }
    }

    std::vector<char const*> exec_args{unparsed_tokens.size()+1};

    int i = 0;
    for (auto const& arg : unparsed_tokens)
        exec_args[i++] = arg.c_str();

    execvp(exec_args[0], const_cast<char*const*>(exec_args.data()));

    std::cerr << "Failed to execute client (" << exec_args[0] << ") error: " << strerror(errno) << '\n';
    return EXIT_FAILURE;
}
catch (std::exception const& x)
{
    std::cerr << "ERROR: " << x.what() << '\n';
    return EXIT_FAILURE;
}
