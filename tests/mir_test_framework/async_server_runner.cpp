/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir_test_framework/async_server_runner.h"
#include "mir_test_framework/command_line_server_configuration.h"

#include "mir/fd.h"
#include <mir/logging/dumb_console_logger.h>
#include <mir/logging/file_logger.h>
#include <mir/logging/multi_logger.h>
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include <mir/report_exception.h>
#include <mir/thread_name.h>
#include "mir/test/doubles/null_logger.h"
#include "mir_test_framework/testing_server_configuration.h"

#include <boost/throw_exception.hpp>

#include <gtest/gtest.h>

namespace ml = mir::logging;
namespace mo = mir::options;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
namespace mt = mir::test;

namespace
{
std::chrono::seconds const timeout{20};

std::string const get_log_filename(const ::testing::TestInfo* test_info)
{
    std::ostringstream filename;
    filename << "/tmp/" << test_info->test_case_name() << "_" << test_info->name() << "_server.log";
    return filename.str();
}
}

mtf::AsyncServerRunner::AsyncServerRunner()
: output_filename{get_log_filename(::testing::UnitTest::GetInstance()->current_test_info())}
{
    unsetenv("WAYLAND_DISPLAY");    // We don't want to conflict with any existing Wayland server
    configure_from_commandline(server);

    server.add_configuration_option(mtd::logging_opt, mtd::logging_descr, false);
    server.override_the_logger([&]() -> std::shared_ptr<ml::Logger>
        {
            std::ofstream out{output_filename};
            if (out.good())
            {
                std::cerr << "Saving server logs to: " << output_filename << std::endl;
                if (server.get_options()->get<bool>(mtd::logging_opt))
                {
                    return std::make_shared<ml::MultiLogger>(
                        std::initializer_list<std::shared_ptr<ml::Logger>>{
                            std::make_shared<ml::DumbConsoleLogger>(),
                            std::make_shared<ml::FileLogger>(std::move(out))
                        }
                    );
                }
                else
                {
                    return std::make_shared<ml::FileLogger>(std::move(out));
                }
            }
            else
            {
                std::cerr << "Failed to open log file: " << output_filename << std::endl;
                if (!server.get_options()->get<bool>(mtd::logging_opt))
                {
                    return std::make_shared<mtd::NullLogger>();
                }
            }

            // If we return null then the default will be used
            return {};
        });
}

void mtf::AsyncServerRunner::add_to_environment(char const* key, char const* value)
{
    env.emplace_back(key, value);
}

void mtf::AsyncServerRunner::start_server()
{

    mir::test::CrossProcessSync cross_process_sync;
    server.override_the_server_status_listener(
        [cross_process_sync = std::move(cross_process_sync)]
        {
            return std::make_shared<TestingServerStatusListener>(
                cross_process_sync,
                []
                {
                });
        });

    mt::AutoJoinThread t([this]
        {
            try
            {
                mir::set_thread_name("mtf/AsyncServer");

                server.add_init_callback([this]
                    {
                        auto const main_loop = server.the_main_loop();
                        // By enqueuing the notification code in the main loop, we are
                        // ensuring that the server has really and fully started before
                        // leaving start_mir_server().
                        main_loop->enqueue(
                            this,
                            [this]
                            {
                                {
                                    std::lock_guard lock(mutex);
                                    server_running = true;
                                }
                                started.notify_one();
                            });
                    });

                server.apply_settings();
                server.run();
            }
            catch (std::exception const& e)
            {
                std::ostringstream error;
                mir::report_exception(error);
                FAIL() << error.str();
            }
            {
                std::lock_guard lock(mutex);
                server_running = false;
            }
            started.notify_one();
        });

    std::unique_lock lock(mutex);
    started.wait_for(lock, timeout, [this] { return server_running; });

    std::dynamic_pointer_cast<TestingServerStatusListener>(server.the_server_status_listener())
        ->server_started_sync.wait_for_signal_ready_for(timeout);

    if (!server_running)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to start server thread"});
    }
    server_thread = std::move(t);
}

void mtf::AsyncServerRunner::stop_server()
{
    server.stop();
    wait_for_server_exit();
}

void mtf::AsyncServerRunner::wait_for_server_exit()
{
    std::unique_lock lock(mutex);
    started.wait_for(lock, timeout, [&] { return !server_running; });

    if (server_running)
    {
        BOOST_THROW_EXCEPTION(std::logic_error{"stop_server() failed to stop server"});
    }
    server_thread.stop();
}

mtf::AsyncServerRunner::~AsyncServerRunner() noexcept
{
}
