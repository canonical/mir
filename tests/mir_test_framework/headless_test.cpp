/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/headless_test.h"

#include "mir/main_loop.h"

#include <boost/throw_exception.hpp>

namespace mtf = mir_test_framework;

namespace
{
std::chrono::seconds const timeout{10};
}

mtf::HeadlessTest::HeadlessTest()
{
    add_to_environment("MIR_SERVER_PLATFORM_GRAPHICS_LIB", "libmirplatformstub.so");
}

void mtf::HeadlessTest::add_to_environment(char const* key, char const* value)
{
    env.emplace_back(key, value);
}

void mtf::HeadlessTest::start_server()
{
    server.add_init_callback([&]
        {
            auto const main_loop = server.the_main_loop();
            // By enqueuing the notification code in the main loop, we are
            // ensuring that the server has really and fully started before
            // leaving start_mir_server().
            main_loop->enqueue(
                this,
                [&]
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    server_running = true;
                    started.notify_one();
                });
        });

    server_thread = std::thread([&]
        {
            try
            {
                server.run();
            }
            catch (std::exception const& e)
            {
                FAIL() << e.what();
            }
            std::lock_guard<std::mutex> lock(mutex);
            server_running = false;
            started.notify_one();
        });

    std::unique_lock<std::mutex> lock(mutex);
    started.wait_for(lock, timeout, [&] { return server_running; });

    if (!server_running)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to start server thread"});
    }
}

void mtf::HeadlessTest::stop_server()
{
    connections.clear();

    std::unique_lock<std::mutex> lock(mutex);
    if (server_running)
    {
        server.stop();
        started.wait_for(lock, timeout, [&] { return !server_running; });
    }

    if (server_running)
    {
        BOOST_THROW_EXCEPTION(std::logic_error{"stop_server() failed to stop server"});
    }
}

mtf::HeadlessTest::~HeadlessTest() noexcept
{
    if (server_thread.joinable()) server_thread.join();
}

auto mtf::HeadlessTest::new_connection() -> std::string
{
    char connect_string[64] = {0};
    auto const client_socket = server.open_client_socket();
    connections.push_back(client_socket);
    sprintf(connect_string, "fd://%d", client_socket.operator int());
    return connect_string;
}
