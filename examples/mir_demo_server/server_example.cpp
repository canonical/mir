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
#include <miral/keymap.h>
#include <miral/live_config.h>
#include <miral/live_config_ini_file.h>
#include <miral/minimal_window_manager.h>
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
#include <miral/application_switcher.h>
#include <miral/internal_client.h>

#include "mir/abnormal_exit.h"
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include "mir/report_exception.h"
#include "mir/server.h"

#include <boost/exception/diagnostic_information.hpp>

#include <chrono>
#include <cstdlib>
#include <linux/input-event-codes.h>

#include "miral/sticky_keys.h"

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

    miral::live_config::IniFile config_store;
    runner.set_exception_handler(exception_handler);

    miral::CursorScale cursor_scale{config_store};
    miral::OutputFilter output_filter{config_store};
    miral::InputConfiguration input_configuration{config_store};
    miral::BounceKeys bounce_keys{config_store};
    miral::SlowKeys slow_keys{config_store};
    miral::StickyKeys sticky_keys{config_store};
    miral::Keymap keymap{config_store};
    miral::HoverClick hover_click{config_store};
    miral::ApplicationSwitcher application_switcher;

    miral::ConfigFile config_file{
        runner,
        "mir_demo_server.live-config",
        miral::ConfigFile::Mode::reload_on_change,
        [&config_store](auto&... args){ config_store.load_file(args...); }};

    std::function<void()> shutdown_hook{[]{}};
    runner.add_stop_callback([&] { shutdown_hook(); application_switcher.stop(); });

    InputFilters input_filters;
    me::TestClientRunner test_runner;

    miral::WaylandExtensions wayland_extensions;

    for (auto const& ext : wayland_extensions.all_supported())
        wayland_extensions.enable(ext);

    // The following filter triggers the application selector internal application
    // on "Ctrl + Tab" and "Ctrl + Shift + Tab". Note that this does NOT use
    // "Alt + Tab" as that is claimed by the MinimalWindowManager for the time being.
    auto const application_switcher_filter = [application_switcher=application_switcher, is_running = false](MirKeyboardEvent const* key_event) mutable
    {
        if (mir_keyboard_event_action(key_event) == mir_keyboard_action_down)
        {
            auto const modifiers = mir_keyboard_event_modifiers(key_event);
            if (modifiers & mir_input_event_modifier_ctrl)
            {
                auto const scancode = mir_keyboard_event_scan_code(key_event);
                if (modifiers & mir_input_event_modifier_shift)
                {
                    if (scancode == KEY_TAB)
                    {
                        application_switcher.prev_app();
                        is_running = true;
                        return true;
                    }
                    else if (scancode == KEY_GRAVE)
                    {
                        application_switcher.prev_window();
                        is_running = true;
                        return true;
                    }
                }
                if (scancode == KEY_TAB)
                {
                    application_switcher.next_app();
                    is_running = true;
                    return true;
                }
                else if (scancode == KEY_GRAVE)
                {
                    application_switcher.next_window();
                    is_running = true;
                    return true;
                }
                else if (scancode == KEY_ESC && is_running)
                {
                    application_switcher.cancel();
                    is_running = false;
                    return true;
                }
            }
        }
        else if (mir_keyboard_event_action(key_event) == mir_keyboard_action_up)
        {
            auto const scancode = mir_keyboard_event_scan_code(key_event);
            if ((scancode == KEY_LEFTCTRL || scancode == KEY_RIGHTCTRL) && is_running)
            {
                application_switcher.confirm();
                is_running = false;
                return true;
            }
        }

        return false;
    };

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
        output_filter,
        input_configuration,
        cursor_scale,
        bounce_keys,
        slow_keys,
        sticky_keys,
        hover_click,
        keymap,
        miral::AppendKeyboardEventFilter{application_switcher_filter},
        miral::StartupInternalClient{application_switcher},
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
