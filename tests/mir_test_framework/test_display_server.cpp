/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include <miral/test_display_server.h>

#include <miral/command_line_option.h>
#include <miral/minimal_window_manager.h>
#include <miral/set_window_management_policy.h>

#include <mir/client/connection.h>

#include <mir_test_framework/executable_path.h>
#include <mir_test_framework/headless_display_buffer_compositor_factory.h>
#include <mir/test/doubles/null_logger.h>

#include <mir/fd.h>
#include <mir/main_loop.h>
#include <mir/server.h>
#include <mir/options/option.h>

#include <boost/throw_exception.hpp>

using namespace miral;
using namespace std::chrono_literals;
namespace mtf = mir_test_framework;
namespace msh = mir::shell;
namespace ml = mir::logging;
namespace mtd = mir::test::doubles;

namespace
{
std::chrono::seconds const timeout{20};
char const* dummy_args[2] = { "TestServer", nullptr };
char const* const trace_option = "window-management-trace";
}

miral::TestDisplayServer::TestDisplayServer() :
    runner{1, dummy_args}
{
    add_to_environment("MIR_SERVER_PLATFORM_GRAPHICS_LIB", mtf::server_platform("graphics-dummy.so").c_str());
    add_to_environment("MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str());
    add_to_environment("MIR_SERVER_NO_FILE", "on");
    add_to_environment("MIR_SERVER_CONSOLE_PROVIDER", "none");
}

miral::TestDisplayServer::~TestDisplayServer() = default;

auto miral::TestDisplayServer::build_window_manager_policy(WindowManagerTools const& tools)
-> std::unique_ptr<WindowManagementPolicy>
{
    return std::make_unique<MinimalWindowManager>(tools);
}

void miral::TestDisplayServer::start_server()
{
    mir::test::AutoJoinThread t([this]
         {
            SetWindowManagementPolicy wm_policy{
                [this](WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
                {
                    this->tools = tools;
                    return build_window_manager_policy(tools);
                }};

            auto init = [this](mir::Server& server)
                {
                    server.add_configuration_option(trace_option, "log trace message", mir::OptionType::null);
                    server.add_configuration_option(mtd::logging_opt, mtd::logging_descr, false);

                    server.add_init_callback([&]
                        {
                            auto const main_loop = server.the_main_loop();
                            // By enqueuing the notification code in the main loop, we are
                            // ensuring that the server has really and fully started before
                            // leaving start_mir_server().
                            main_loop->enqueue(this, [&]
                                {
                                     std::lock_guard<std::mutex> lock(mutex);
                                     server_running = &server;
                                     started.notify_one();
                                });
                        });

                    server.override_the_display_buffer_compositor_factory([]
                        {
                            return std::make_shared<mtf::HeadlessDisplayBufferCompositorFactory>();
                        });

                    server.override_the_logger([&]()
                        {
                            std::shared_ptr<ml::Logger> result{};

                            if (!server.get_options()->get<bool>(mtd::logging_opt))
                                result = std::make_shared<mtd::NullLogger>();

                            return result;
                        });
                };

            try
            {
                runner.run_with({wm_policy, init, init_server});
            }
            catch (std::exception const& e)
            {
                mir::fatal_error(e.what());
            }

            std::lock_guard<std::mutex> lock(mutex);
            server_running = nullptr;
            started.notify_one();
         });

    std::unique_lock<std::mutex> lock(mutex);
    started.wait_for(lock, timeout, [&] { return server_running; });

    if (!server_running)
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to start server thread"});

    server_thread = std::move(t);
}

void miral::TestDisplayServer::stop_server()
{
    std::unique_lock<std::mutex> lock(mutex);

    runner.stop();

    started.wait_for(lock, timeout, [&] { return !server_running; });

    if (server_running)
        BOOST_THROW_EXCEPTION(std::logic_error{"Failed to stop server"});

    server_thread.stop();
}

auto miral::TestDisplayServer::connect_client(std::string name) -> mir::client::Connection
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!server_running)
        BOOST_THROW_EXCEPTION(std::runtime_error{"Server not running"});

    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", dup(server_running->open_client_socket()));

    return mir::client::Connection{mir_connect_sync(connect_string, name.c_str())};
}

void miral::TestDisplayServer::invoke_tools(std::function<void(WindowManagerTools& tools)> const& f)
{
    tools.invoke_under_lock([&]{f(tools); });
}

void TestDisplayServer::add_server_init(std::function<void(mir::Server&)>&& init)
{
    auto temp = [old_init=init_server, new_init=std::move(init)](mir::Server& server)
        {
            old_init(server);
            new_init(server);
        };

    init_server = temp;
}

void TestDisplayServer::add_start_callback(std::function<void()> const& start_callback)
{
    runner.add_start_callback(start_callback);
}

void TestDisplayServer::add_stop_callback(std::function<void()> const& stop_callback)
{
    runner.add_stop_callback(stop_callback);
}

void TestDisplayServer::set_exception_handler(std::function<void()> const& handler)
{
    runner.set_exception_handler(handler);
}

void miral::TestRuntimeEnvironment::add_to_environment(char const* key, char const* value)
{
    env.emplace_back(key, value);
}
