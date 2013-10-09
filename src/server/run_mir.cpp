/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "mir/run_mir.h"
#include "mir/display_server.h"
#include "mir/main_loop.h"
#include "mir/server_configuration.h"
#include "mir/frontend/connector.h"

#include <csignal>
#include <cstdlib>
#include <cassert>

namespace
{
std::weak_ptr<mir::frontend::Connector> weak_connector;

extern "C" void delete_endpoint()
{
    if (auto connector = weak_connector.lock())
    {
        weak_connector.reset();
        connector->remove_endpoint();
    }
}

extern "C" { typedef void (*sig_handler)(int); }

volatile sig_handler old_handler[SIGUNUSED]  = { nullptr };

extern "C" void fatal_signal_cleanup(int sig)
{
    delete_endpoint();

    signal(sig, old_handler[sig]);
    raise(sig);
}
}

void mir::run_mir(ServerConfiguration& config, std::function<void(DisplayServer&)> init)
{
    DisplayServer* server_ptr{nullptr};
    auto main_loop = config.the_main_loop();

    main_loop->register_signal_handler(
        {SIGINT, SIGTERM},
        [&server_ptr](int)
        {
            delete_endpoint();
            assert(server_ptr);
            server_ptr->stop();
        });

    DisplayServer server(config);
    server_ptr = &server;

    weak_connector = config.the_connector();

    for (auto sig : { SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS })
        old_handler[sig] = signal(sig, fatal_signal_cleanup);

    std::atexit(&delete_endpoint);

    init(server);
    server.run();
}
