/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "mir_test_framework/server_runner.h"

#include "mir/default_server_configuration.h"
#include "mir/display_server.h"
#include "mir/frontend/connector.h"
#include "mir/run_mir.h"
#include "mir/main_loop.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>

#include <condition_variable>
#include <mutex>

namespace mtf = mir_test_framework;

namespace
{
char const* const env_no_file = "MIR_SERVER_NO_FILE";
}

mtf::ServerRunner::ServerRunner() :
    old_env(getenv(env_no_file))
{
    if (!old_env) setenv(env_no_file, "", true);
}

void mtf::ServerRunner::start_server()
{
    auto const ml = start_mir_server();
    if (ml == nullptr)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to start server thread"});
    }

    std::lock_guard<std::mutex> main_loop_lock{main_loop_mutex};
    main_loop = ml;
}

std::string mtf::ServerRunner::new_connection()
{
    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", server_config().the_connector()->client_socket_fd());
    return connect_string;
}

std::string mtf::ServerRunner::new_prompt_connection()
{
    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", server_config().the_prompt_connector()->client_socket_fd());
    return connect_string;
}

void mtf::ServerRunner::stop_server()
{
    decltype(main_loop) ml;

    {
        std::lock_guard<std::mutex> main_loop_lock{main_loop_mutex};
        ml = main_loop;
    }

    if (ml == nullptr)
    {
        BOOST_THROW_EXCEPTION(std::logic_error{"stop_server() called without calling start_server()?"});
    }
    ml->stop();
    if (server_thread.joinable()) server_thread.join();
}

mtf::ServerRunner::~ServerRunner()
{
    if (server_thread.joinable()) server_thread.join();

    if (!old_env) unsetenv(env_no_file);
}

std::shared_ptr<mir::MainLoop> mtf::ServerRunner::start_mir_server()
{
    std::mutex mutex;
    std::condition_variable started_cv;
    bool started{false};
    auto const ml = server_config().the_main_loop();
    mir::logging::set_logger(server_config().the_logger());

    server_thread = std::thread([&]
    {
        try
        {
            mir::run_mir(server_config(), [&](mir::DisplayServer&)
            {
                // By enqueuing the notification code in the main loop, we are
                // ensuring that the server has really and fully started before
                // leaving start_mir_server().
                ml->enqueue(
                    this,
                    [&]
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        started = true;
                        started_cv.notify_one();
                    });
            });
        }
        catch (std::exception const& e)
        {
            FAIL() << e.what();
        }
    });

    std::unique_lock<std::mutex> lock(mutex);
    started_cv.wait_for(lock, std::chrono::seconds{30}, [&]{ return started; });

    return ml;
}
