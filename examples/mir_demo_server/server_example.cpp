/*
 * Copyright © Canonical Ltd.
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
 */

#include <miral/config_file_store_adapter.h>
#include "server_example_input_event_filter.h"
#include "server_example_input_filter.h"
#include "server_example_test_client.h"

#include <miral/cursor_theme.h>
#include <miral/display_configuration_option.h>
#include <miral/input_configuration.h>
#include <miral/keymap.h>
#include <miral/live_config.h>
#include <miral/live_config_ini_file.h>
#include <miral/floating_window_manager.h>
#include <miral/config_file.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/wayland_extensions.h>
#include <miral/x11_support.h>
#include <miral/cursor_scale.h>
#include <miral/output_filter.h>
#include <miral/bounce_keys.h>
#include <miral/slow_keys.h>
#include <miral/hover_click.h>
#include <miral/append_keyboard_event_filter.h>

#include <mir/abnormal_exit.h>
#include <mir/main_loop.h>
#include <mir/options/option.h>
#include <mir/report_exception.h>
#include <mir/server.h>

#include <boost/exception/diagnostic_information.hpp>

#include <chrono>
#include <cstdlib>
#include <format>
#include <linux/input-event-codes.h>

#include <miral/sticky_keys.h>

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
    auto const command = std::format(
        "/usr/share/apport/recoverable_problem --pid {}", getpid());
    char const options[] = "we";
    char const key[] = "UnhandledException";
    auto const value = boost::diagnostic_information(error);

    if (auto const output = popen(command.c_str(), options))
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

    miral::live_config::ConfigAggregator config_aggregator{};
    miral::ConfigFileStoreAdapter adapter{
        config_aggregator,
        [](std::unique_ptr<std::istream> stream, std::filesystem::path const& path)
        {
            auto parser = std::make_shared<miral::live_config::IniFile>();
            return miral::live_config::ConfigAggregator::Source{
                parser,
                [parser, stream = std::move(stream), path]()
                {
                    parser->load_file(*stream, path);
                },
                path,
            };
        }};

    miral::live_config::IniFile ini_file;
    runner.set_exception_handler(exception_handler);

    miral::CursorScale cursor_scale{config_aggregator};
    miral::OutputFilter output_filter{config_aggregator};
    miral::InputConfiguration input_configuration{config_aggregator};
    miral::BounceKeys bounce_keys{config_aggregator};
    miral::SlowKeys slow_keys{config_aggregator};
    miral::StickyKeys sticky_keys{config_aggregator};
    miral::Keymap keymap{config_aggregator};
    miral::HoverClick hover_click{config_aggregator};

    miral::ConfigFile config_file{
        runner,
        "mir_demo_server.live-config",
        miral::ConfigFile::Mode::reload_on_change,
        [&adapter](auto args) { adapter(args); },
    };

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); });

    InputFilters input_filters;
    me::TestClientRunner test_runner;

    miral::WaylandExtensions wayland_extensions;

    for (auto const& ext : wayland_extensions.all_supported())
        wayland_extensions.enable(ext);

    auto const server_exit_status = runner.run_with({
        // example options for display layout, logging and timeout
        miral::display_configuration_options,
        miral::X11Support{},
        wayland_extensions,
        miral::set_window_management_policy<miral::FloatingWindowManager>(),
        add_timeout_option_to,
        miral::CursorTheme{"default:DMZ-White"},
        input_filters,
        test_runner,
        output_filter,
        input_configuration,
        cursor_scale,
        bounce_keys,
        slow_keys,
        sticky_keys,
        hover_click,
        keymap,
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
