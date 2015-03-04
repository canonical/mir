/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/main_loop.h"
#include "mir/fd.h"

#include "mir_test_framework/nested_mir_runner.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;

using namespace ::testing;

namespace
{
struct FakeCommandLine
{
    static int const argc = 7;
    char const* argv[argc];

    FakeCommandLine(std::string const& host_socket)
    {
        char const** to = argv;
        for(auto from : { "dummy-exe-name", "--file", "NestedServer", "--host-socket", host_socket.c_str(), "--enable-input", "off"})
        {
            *to++ = from;
        }

        EXPECT_THAT(to - argv, Eq(argc)); // Check the array size matches parameter list
    }
};

std::chrono::seconds const timeout{10};
}

mtf::NestedMirRunner::NestedMirRunner(std::function<void(mir::Server&)> do_setup, std::string const& connection_string)
{
    FakeCommandLine nested_command_line(connection_string);
    set_command_line(nested_command_line.argc, nested_command_line.argv);

    do_setup(*this);

    add_init_callback([&]
    {
        auto const main_loop = the_main_loop();
        // By enqueuing the notification code in the main loop, we are
        // ensuring that the server has really and fully started before
        // leaving start_mir_server().
        main_loop->enqueue(
            this,
            [&]
            {
                std::lock_guard<std::mutex> lock(nested_mutex);
                nested_server_running = true;
                nested_started.notify_one();
            });
    });
    apply_settings();
    nested_server_thread = std::thread([&]
    {
        try
        {
            run();
        }
        catch (std::exception const& e)
        {
            FAIL() << e.what();
        }
        std::lock_guard<std::mutex> lock(nested_mutex);
        nested_server_running = false;
        nested_started.notify_one();
    });
    std::unique_lock<std::mutex> lock(nested_mutex);
    nested_started.wait_for(lock, timeout, [&] { return nested_server_running; });
    if (!nested_server_running)
    {
       throw std::runtime_error{"Failed to start nested server"};
    }
}

mtf::NestedMirRunner::~NestedMirRunner()
{
    stop();

    std::unique_lock<std::mutex> lock(nested_mutex);
    nested_started.wait_for(lock, timeout, [&] { return !nested_server_running; });

    EXPECT_FALSE(nested_server_running);

    connections.clear();

    if (nested_server_thread.joinable()) nested_server_thread.join();
}

std::string mtf::NestedMirRunner::new_connection()
{
    auto fd = open_client_socket();
    connections.push_back(fd);

    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", fd.operator int());
    return connect_string;
}
