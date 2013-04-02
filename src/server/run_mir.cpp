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
#include "mir/signal_dispatcher.h"

#include <atomic>
#include <thread>
#include <csignal>

namespace
{
std::atomic<mir::DisplayServer*> signal_display_server;

void signal_terminate(int)
{
    while (!signal_display_server.load())
        std::this_thread::yield();

    signal_display_server.load()->stop();
}
}

void mir::run_mir(ServerConfiguration& config, std::function<void(DisplayServer&)> init)
{
    auto const sdp = SignalDispatcher::instance();
    sdp->enable_for(SIGINT);
    sdp->enable_for(SIGTERM);
    sdp->signal_channel().connect(&signal_terminate);

    DisplayServer server(config);
    signal_display_server.store(&server);

    init(server);
    server.run();
}
