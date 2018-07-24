/*
 * Copyright © 2012-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "server_example_log_options.h"
#include "server_example_input_event_filter.h"
#include "server_example_input_filter.h"
#include "server_example_host_lifecycle_event_listener.h"
#include "server_example_custom_compositor.h"
#include "server_example_test_client.h"
#include "server_example_input_device_config.h"

#include "tiling_window_manager.h"
#include "floating_window_manager.h"
#include "titlebar_config.h"
#include "sw_splash.h"

#include <miral/cursor_theme.h>
#include <miral/display_configuration_option.h>
#include <miral/runner.h>
#include <miral/window_management_options.h>
#include <miral/x11_support.h>

#include "mir/abnormal_exit.h"
#include "mir/server.h"
#include "mir/main_loop.h"
#include "mir/report_exception.h"
#include "mir/options/option.h"

#include <boost/exception/diagnostic_information.hpp>

#include <chrono>
#include <cstdlib>

namespace mir { class AbnormalExit; }

namespace me = mir::examples;

///\example server_example.cpp
/// A simple server illustrating several customisations

namespace
{
void add_timeout_option_to(mir::Server& server)
{
    static const char* const timeout_opt = "timeout";
    static const char* const timeout_descr = "Seconds to run before exiting";

    server.add_configuration_option(timeout_opt, timeout_descr, mir::OptionType::integer);

    server.add_init_callback([&server]
    {
        const auto options = server.get_options();
        if (options->is_set(timeout_opt))
        {
            static auto const exit_action = server.the_main_loop()->create_alarm([&server] { server.stop(); });
            exit_action->reschedule_in(std::chrono::seconds(options->get<int>(timeout_opt)));
        }
    });
}


// Create some input filters (we need to keep them or they deactivate)
struct InputFilters
{
    void operator()(mir::Server& server)
    {
        quit_filter = me::make_quit_filter_for(server);
        printing_filter = me::make_printing_input_filter_for(server);
        screen_rotation_filter = me::make_screen_rotation_filter_for(server);
    }

private:
    std::shared_ptr<mir::input::EventFilter> quit_filter;
    std::shared_ptr<mir::input::EventFilter> printing_filter;
    std::shared_ptr<mir::input::EventFilter> screen_rotation_filter;
};

void exception_handler()
try
{
    mir::report_exception();
    throw;
}
catch (mir::AbnormalExit const& /*error*/)
{
}
catch (std::exception const& error)
{
    char const command_fmt[] = "/usr/share/apport/recoverable_problem --pid %d";
    char command[sizeof(command_fmt)+32];
    snprintf(command, sizeof(command), command_fmt, getpid());
    char const options[] = "we";
    char const key[] = "UnhandledException";
    auto const value = boost::diagnostic_information(error);

    if (auto const output = popen(command, options))
    {
        fwrite(key, sizeof(key), 1, output);            // the null terminator is used intentionally as a separator
        fwrite(value.c_str(), value.size(), 1, output);
        pclose(output);
    }
}
catch (...)
{
}
}

int main(int argc, char const* argv[])
try
{
    miral::MirRunner runner{argc, argv, "mir/mir_demo_server.config"};

    runner.set_exception_handler(exception_handler);

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    SwSplash spinner;
    miral::InternalClientLauncher launcher;
    miral::WindowManagerOptions window_managers
        {
            miral::add_window_manager_policy<FloatingWindowManagerPolicy>("floating", spinner, launcher, shutdown_hook),
            miral::add_window_manager_policy<TilingWindowManagerPolicy>("tiling", spinner, launcher),
        };

    InputFilters input_filters;
    me::TestClientRunner test_runner;

    auto const server_exit_status = runner.run_with({
        // example options for display layout, logging and timeout
        miral::display_configuration_options,
        me::add_log_host_lifecycle_option_to,
        me::add_glog_options_to,
        miral::StartupInternalClient{"Intro", spinner},
        miral::X11Support{},
        launcher,
        window_managers,
        me::add_custom_compositor_option_to,
        me::add_input_device_configuration_options_to,
        add_timeout_option_to,
        miral::CursorTheme{"default:DMZ-White"},
        input_filters,
        test_runner
    });

    // Propagate any test failure
    if (test_runner.test_failed())
    {
        return EXIT_FAILURE;
    }

    return server_exit_status;
}
catch (...)
{
    mir::report_exception();
    return EXIT_FAILURE;
}
