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

#include "server_example_input_event_filter.h"
#include "server_example_input_filter.h"
#include "server_example_test_client.h"

#include <miral/cursor_theme.h>
#include <miral/display_configuration_option.h>
#include <miral/input_configuration.h>
#include <miral/minimal_window_manager.h>
#include <miral/config_file.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/wayland_extensions.h>
#include <miral/x11_support.h>

#include "mir/abnormal_exit.h"
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include "mir/report_exception.h"
#include "mir/server.h"
#include "mir/log.h"

#include <boost/exception/diagnostic_information.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>

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

class InputConfigFile
{
public:
    InputConfigFile(miral::MirRunner& runner, std::filesystem::path file) :
        config_file{
            runner,
            file,
            miral::ConfigFile::Mode::reload_on_change,
            [this](std::istream& istream, std::filesystem::path const& path) {loader(istream, path); }}
    {
        runner.add_start_callback([this]
            {
                std::lock_guard lock{config_mutex};
                apply_config();
            });
    }

    void operator()(mir::Server& server)
    {
        input_configuration(server);
    }

private:
    miral::InputConfiguration input_configuration;
    miral::InputConfiguration::Mouse mouse = input_configuration.mouse();
    miral::InputConfiguration::Touchpad touchpad = input_configuration.touchpad();
    miral::InputConfiguration::Keyboard keyboard = input_configuration.keyboard();
    std::mutex config_mutex;
    miral::ConfigFile config_file;

    void apply_config()
    {
        input_configuration.mouse(mouse);
        input_configuration.touchpad(touchpad);
        input_configuration.keyboard(keyboard);
    };

    void loader(std::istream& in, std::filesystem::path const& path)
    {
        std::cout << "** Reloading: " << path << std::endl;

        std::lock_guard lock{config_mutex};

        for (std::string line; std::getline(in, line);)
        {
            std::cout << line << std::endl;

            if (line == "mir_pointer_handedness_right")
                mouse.handedness(mir_pointer_handedness_right);
            if (line == "mir_pointer_handedness_left")
                mouse.handedness(mir_pointer_handedness_left);

            if (line == "mir_touchpad_scroll_mode_none")
                touchpad.scroll_mode(mir_touchpad_scroll_mode_none);
            if (line == "mir_touchpad_scroll_mode_two_finger_scroll")
                touchpad.scroll_mode(mir_touchpad_scroll_mode_two_finger_scroll);
            if (line == "mir_touchpad_scroll_mode_edge_scroll")
                touchpad.scroll_mode(mir_touchpad_scroll_mode_edge_scroll);
            if (line == "mir_touchpad_scroll_mode_button_down_scroll")
                touchpad.scroll_mode(mir_touchpad_scroll_mode_button_down_scroll);

            if (line.contains("="))
            {
                auto const eq = line.find_first_of("=");
                auto const key = line.substr(0, eq);
                auto const value = line.substr(eq+1);

                auto const parse_and_validate = [](std::string const& key, std::string_view val) -> std::optional<int>
                {
                    auto const int_val = std::atoi(val.data());
                    if (int_val < 0)
                    {
                        mir::log_warning(
                            "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                            key.c_str(), int_val);
                        return std::nullopt;
                    }

                    return int_val;
                };

                if (key == "repeat_rate")
                {
                    auto const parsed = parse_and_validate(key, value);
                    if (parsed)
                        keyboard.set_repeat_rate(*parsed);
                }

                if (key == "repeat_delay")
                {
                    auto const parsed = parse_and_validate(key, value);
                    if (parsed)
                        keyboard.set_repeat_delay(*parsed);
                }
            }
        }

        apply_config();
    }
};
}

int main(int argc, char const* argv[])
try
{
    miral::MirRunner runner{argc, argv, "mir/mir_demo_server.config"};

    InputConfigFile input_configuration{runner, "mir_demo_server.input"};
    runner.set_exception_handler(exception_handler);

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
        miral::set_window_management_policy<miral::MinimalWindowManager>(),
        add_timeout_option_to,
        miral::CursorTheme{"default:DMZ-White"},
        input_filters,
        test_runner,
        std::ref(input_configuration),
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
