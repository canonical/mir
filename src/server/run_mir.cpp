/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/run_mir.h"
#include "mir/display_server.h"

#include <thread>
#include <atomic>
#include <csignal>
#include <future>

namespace
{
std::atomic<mir::DisplayServer*> signal_display_server;

extern "C" void signal_terminate(int)
{
    while (!signal_display_server.load())
        std::this_thread::yield();

    signal_display_server.load()->stop();
}
}

void mir::run_mir(ServerConfiguration& config, std::function<void(DisplayServer&)> init)
{
    signal(SIGINT, signal_terminate);
    signal(SIGTERM, signal_terminate);
    signal(SIGPIPE, SIG_IGN);

    // Run on a separate thread as the signal handler can run on this one
    std::future<void> future = std::async(std::launch::async,
        [&]()
        {
            mir::DisplayServer server(config);

            signal_display_server.store(&server);

            init(server);

            server.run();
        });

    future.wait();
}
