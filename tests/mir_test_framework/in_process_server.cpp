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

#include "mir_test_framework/in_process_server.h"

#include "mir/default_server_configuration.h"
#include "mir/display_server.h"
#include "mir/frontend/connector.h"
#include "mir/run_mir.h"

#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

namespace mtf = mir_test_framework;

namespace
{
char const* const env_no_file = "MIR_SERVER_NO_FILE";
}

mtf::InProcessServer::InProcessServer() :
    old_env(getenv(env_no_file))
{
    if (!old_env) setenv(env_no_file, "", true);
}

void mtf::InProcessServer::SetUp()
{
    display_server = start_mir_server();
    ASSERT_TRUE(display_server);
}

std::string mtf::InProcessServer::new_connection()
{
    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", server_config().the_connector()->client_socket_fd());
    return connect_string;
}

void mtf::InProcessServer::TearDown()
{
    ASSERT_TRUE(display_server)
        << "Did you override SetUp() and forget to call mtf::InProcessServer::SetUp()?";
    display_server->stop();
}

mtf::InProcessServer::~InProcessServer()
{
    if (server_thread.joinable()) server_thread.join();

    if (!old_env) unsetenv(env_no_file);
}

mir::DisplayServer* mtf::InProcessServer::start_mir_server()
{
    std::mutex mutex;
    std::condition_variable cv;
    mir::DisplayServer* result{nullptr};

    server_thread = std::thread([&]
    {
        try
        {
            mir::run_mir(server_config(), [&](mir::DisplayServer& ds)
            {
                std::unique_lock<std::mutex> lock(mutex);
                result = &ds;
                cv.notify_one();
            });
        }
        catch (std::exception const& e)
        {
            FAIL() << e.what();
        }
    });

    using namespace std::chrono;
    auto const time_limit = system_clock::now() + seconds(2);

    std::unique_lock<std::mutex> lock(mutex);

    while (!result && time_limit > system_clock::now())
        cv.wait_until(lock, time_limit);

    return result;
}
