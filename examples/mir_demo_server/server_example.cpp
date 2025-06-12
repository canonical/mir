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
#include <miral/live_config.h>
#include <miral/minimal_window_manager.h>
#include <miral/config_file.h>
#include <miral/runner.h>
#include <miral/set_window_management_policy.h>
#include <miral/wayland_extensions.h>
#include <miral/x11_support.h>
#include <miral/cursor_scale.h>
#include <miral/output_filter.h>

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
#include <list>
#include <mutex>

namespace mir { class AbnormalExit; }

namespace me = mir::examples;
namespace live_config = miral::live_config;

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

// for illustrative purposes
class DemoConfigFile : public live_config::Store
{
public:
    DemoConfigFile(miral::MirRunner& runner, std::filesystem::path file) :
        init_part2{[&runner, file, this]
        {
            miral::ConfigFile temp
            {
                runner,
                file,
                miral::ConfigFile::Mode::reload_on_change,
                [this](std::istream& istream, std::filesystem::path const& path) {loader(istream, path); }
            };

            std::lock_guard lock{config_mutex};
            config_file = temp;
        }}
    {
    }

    // Allows the configuration to be processed after the handlers are registered
    void handle_initial_config() const
    {
        init_part2();
    }

    void add_int_attribute(live_config::Key const& key, std::string_view description, HandleInt handler) override;
    void add_ints_attribute(live_config::Key const& key, std::string_view description, HandleInts handler) override;
    void add_bool_attribute(live_config::Key const& key, std::string_view description, HandleBool handler) override;
    void add_float_attribute(live_config::Key const& key, std::string_view description, HandleFloat handler) override;
    void add_floats_attribute(live_config::Key const& key, std::string_view description, HandleFloats handler) override;
    void add_string_attribute(live_config::Key const& key, std::string_view description, HandleString handler) override;
    void add_strings_attribute(live_config::Key const& key, std::string_view description, HandleStrings handler) override;

    void add_int_attribute(live_config::Key const& key, std::string_view description, int preset,
        HandleInt handler) override;
    void add_ints_attribute(live_config::Key const& key, std::string_view description, std::span<int const> preset,
                            HandleInts handler) override;
    void add_bool_attribute(live_config::Key const& key, std::string_view description, bool preset,
        HandleBool handler) override;
    void add_float_attribute(live_config::Key const& key, std::string_view description, float preset,
                             HandleFloat handler) override;
    void add_floats_attribute(live_config::Key const& key, std::string_view description, std::span<float const> preset,
                              HandleFloats handler) override;
    void add_string_attribute(live_config::Key const& key, std::string_view description, std::string_view preset,
        HandleString handler) override;
    void add_strings_attribute(live_config::Key const& key, std::string_view description,
        std::span<std::string const> preset, HandleStrings handler) override;

    void on_done(HandleDone handler) override;

private:

    std::map<live_config::Key, HandleString> attribute_handlers;
    std::list<HandleDone> done_handlers;

    std::mutex config_mutex;
    std::optional<miral::ConfigFile> config_file;
    std::function<void()> const init_part2;

    void apply_config()
    {
        for (auto const& handler : done_handlers)
            handler();
    };

    void loader(std::istream& in, std::filesystem::path const& path)
    {
        std::cout << "** Reloading: " << path << std::endl;

        std::lock_guard lock{config_mutex};

        for (std::string line; std::getline(in, line);)
        {
            std::cout << line << std::endl;

            if (line.contains("="))
            {
                auto const eq = line.find_first_of("=");
                auto const key = live_config::Key{line.substr(0, eq)};
                auto const value = line.substr(eq+1);

                if (auto const handler = attribute_handlers.find(key); handler != attribute_handlers.end())
                {
                    handler->second(handler->first, value);
                }
            }
        }

        apply_config();
    }
};

void DemoConfigFile::add_ints_attribute(live_config::Key const& key, std::string_view, HandleInts handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)key;
}

void DemoConfigFile::add_floats_attribute(live_config::Key const& key, std::string_view, HandleFloats handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)key;
}

void DemoConfigFile::add_strings_attribute(live_config::Key const& key, std::string_view, HandleStrings handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)key;
}

void DemoConfigFile::add_int_attribute(live_config::Key const& key, std::string_view description, int preset,
    HandleInt handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::add_ints_attribute(live_config::Key const& key, std::string_view description,
                                        std::span<int const> preset, HandleInts handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::add_bool_attribute(live_config::Key const& key, std::string_view description, bool preset,
    HandleBool handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::add_float_attribute(live_config::Key const& key, std::string_view description, float preset,
                                         HandleFloat handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::add_floats_attribute(live_config::Key const& key, std::string_view description,
                                          std::span<float const> preset, HandleFloats handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::add_string_attribute(live_config::Key const& key, std::string_view description,
    std::string_view preset, HandleString handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::add_strings_attribute(live_config::Key const& key, std::string_view description,
    std::span<std::string const> preset, HandleStrings handler)
{
    // Not implemented: not needed for discussion
    (void)handler; (void)description; (void)key; (void)preset;
}

void DemoConfigFile::on_done(HandleDone handler)
{
    std::lock_guard lock{config_mutex};
    done_handlers.emplace_back(std::move(handler));
}

void DemoConfigFile::add_int_attribute(live_config::Key const& key, std::string_view description, HandleInt handler)
{
    add_string_attribute(key, description, [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        if (val)
        {
            int parsed_val = 0;

            auto const [_, err] = std::from_chars(val->data(), val->data() + val->size(), parsed_val);

            if (err == std::errc{})
            {
                handler(key, parsed_val);
            }
            else
            {
                mir::log_warning(
                    "Config key '%s' has invalid integer value: %s",
                    key.to_string().c_str(),
                    std::format("{}",*val).c_str());
                handler(key, std::nullopt);
            }
        }
        else
        {
            handler(key, std::nullopt);
        }
    });
}

void DemoConfigFile::add_bool_attribute(live_config::Key const& key, std::string_view description, HandleBool handler)
{
    add_string_attribute(key, description, [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        if (val)
        {
            if (*val == "true")
            {
                handler(key, true);
            }
            else if (*val == "false")
            {
                handler(key, false);
            }
            else
            {
                mir::log_warning(
                    "Config key '%s' has invalid integer value: %s",
                    key.to_string().c_str(),
                    std::format("{}",*val).c_str());
                handler(key, std::nullopt);
            }
        }
        else
        {
            handler(key, std::nullopt);
        }
    });
}

void DemoConfigFile::add_float_attribute(live_config::Key const& key, std::string_view description, HandleFloat handler)
{
    add_string_attribute(key, description, [handler](live_config::Key const& key, std::optional<std::string_view> val)
    {
        if (val)
        {
            float parsed_val = 0;

            auto const [_, err] = std::from_chars(val->data(), val->data() + val->size(), parsed_val);

            if (err == std::errc{})
            {
                handler(key, parsed_val);
            }
            else
            {
                mir::log_warning(
                    "Config key '%s' has invalid integer value: %s",
                    key.to_string().c_str(),
                    std::format("{}",*val).c_str());
                handler(key, std::nullopt);
            }
        }
        else
        {
            handler(key, std::nullopt);
        }
    });
}

void DemoConfigFile::add_string_attribute(live_config::Key const& key, std::string_view, HandleString handler)
{
    std::lock_guard lock{config_mutex};
    attribute_handlers[key] = handler;
}
}

int main(int argc, char const* argv[])
try
{
    miral::MirRunner runner{argc, argv, "mir/mir_demo_server.config"};

    DemoConfigFile demo_configuration{runner, "mir_demo_server.live-config"};
    runner.set_exception_handler(exception_handler);

    miral::CursorScale cursor_scale{demo_configuration};
    miral::OutputFilter output_filter{demo_configuration};
    miral::InputConfiguration input_configuration{demo_configuration};
    demo_configuration.handle_initial_config();

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
        output_filter,
        input_configuration,
        cursor_scale,
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
