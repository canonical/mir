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
void (*signal_prev_fn)(int);
void signal_terminate (int )
{
    while (!signal_display_server)
        std::this_thread::yield();

    signal_display_server->stop();
}
}

namespace
{
void run_mir()
{
    // TODO SIGTERM makes better long term sense - but Ctrl-C will do for now.
    signal_prev_fn = signal(SIGINT, signal_terminate);

    DefaultServerConfiguration config;
    DisplayServer server(config);

    signal_display_server = &server;

    std::future<void> future{
        std::async(std::launch::async, std::bind(&DisplayServer::start, &server))};

    future.wait();
}
}
}

int main()
try
{
    mir::run_mir();
    return 0;
}
catch (std::exception const& error)
{
    std::cerr << "ERROR: " << error.what() << std::endl;
    return 1;
}
