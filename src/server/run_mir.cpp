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

#include <csignal>
#include <cassert>

void mir::run_mir(ServerConfiguration& config, std::function<void(DisplayServer&)> init)
{
    DisplayServer* server_ptr{nullptr};
    auto main_loop = config.the_main_loop();

    main_loop->register_signal_handler(
        {SIGINT, SIGTERM},
        [&server_ptr](int)
        {
            assert(server_ptr);
            server_ptr->stop();
        });

    DisplayServer server(config);
    server_ptr = &server;

    init(server);
    server.run();
}
