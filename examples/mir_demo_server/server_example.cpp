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

class ConfigurationKey
{
public:
    ConfigurationKey(std::initializer_list<std::string_view> key) :
        self(std::make_unique<std::vector<std::string>>(key.begin(), key.end()))
    {
    }

    auto as_path() const -> std::vector<std::string>
    {
        return *self;
    }

    auto to_string() const -> std::string
    {
        std::string result;
        for (auto const& element : *self)
        {
            if (!result.empty())
                result += '_';

            result += element;
        }

        return result;
    }

    auto operator<=>(ConfigurationKey const& that) const { return *self <=> *that.self; }

private:
    std::shared_ptr<std::vector<std::string>> self;
};
/// Interface for adding attributes to a configuration tool.
///
/// The handlers should be called when the configuration is updated. There is
/// no requirement to check the previous value has changed.
///
/// The key is passed to the handler as it can be useful (e.g. for diagnostics)
///
/// This could be supported by various backends (an ini file, a YAML node, etc)
class AttributeHandler
{
public:
    using HandleInt = std::function<void(ConfigurationKey const& key, std::optional<int> value)>;
    using HandleBool = std::function<void(ConfigurationKey const& key, std::optional<bool> value)>;
    using HandleFloat = std::function<void(ConfigurationKey const& key, std::optional<float> value)>;
    using HandleString = std::function<void(ConfigurationKey const& key, std::optional<std::string_view> value)>;
    using HandleDone = std::function<void()>;

    virtual void add_int_attribute(ConfigurationKey const& key, HandleInt handler) = 0;
    virtual void add_bool_attribute(ConfigurationKey const& key, HandleBool handler) = 0;
    virtual void add_float_attribute(ConfigurationKey const& key, HandleFloat handler) = 0;
    virtual void add_string_attribute(ConfigurationKey const& key, HandleString handler) = 0;

    /// Called following a set of related updates (e.g. a file reload) to allow
    /// multiple attributes to be updated transactionally
    virtual void on_done(HandleDone handler) = 0;

    virtual ~AttributeHandler() = default;
};

// Struct for illustrative purposes, this would be rolled into miral::OutputFilter
struct OutputFilter : miral::OutputFilter
{
    explicit OutputFilter(AttributeHandler& config_handler)
    {
        config_handler.add_string_attribute({"output_filter"},
                                            [this](ConfigurationKey const& key, std::optional<std::string_view> val)
            {
                MirOutputFilter new_filter = mir_output_filter_none;
                if (val)
                {
                    auto filter_name = *val;
                    if (filter_name == "grayscale")
                    {
                        new_filter = mir_output_filter_grayscale;
                    }
                    else if (filter_name == "invert")
                    {
                        new_filter = mir_output_filter_invert;
                    }
                    else
                    {
                        mir::log_warning(
                            "Config key '%s' has invalid value: %s",
                            key.to_string().c_str(),
                            val->data());
                    }
                }
                filter(new_filter);
            });
    }
};

// Struct for illustrative purposes, this would be rolled into miral::InputConfiguration
struct InputConfiguration : miral::InputConfiguration
{
    struct State
    {
        State(miral::InputConfiguration::Mouse mouse,
        miral::InputConfiguration::Touchpad touchpad,
        miral::InputConfiguration::Keyboard keyboard) :
        mouse{mouse},
        touchpad{touchpad},
        keyboard{keyboard}
        {}

        std::mutex mutex;
        miral::InputConfiguration::Mouse mouse;
        miral::InputConfiguration::Touchpad touchpad;
        miral::InputConfiguration::Keyboard keyboard;
    };

    std::shared_ptr<State> const state = std::make_shared<State>(mouse(), touchpad(), keyboard());

    explicit InputConfiguration(AttributeHandler& config_handler) : miral::InputConfiguration{}
    {
        config_handler.add_string_attribute({"pointer", "handedness"},
                                            [this](ConfigurationKey const& key, std::optional<std::string_view> val)
            {
                if (val)
                {
                    std::lock_guard lock{state->mutex};
                    auto const& value = *val;
                    if (value == "right")
                        state->mouse.handedness(mir_pointer_handedness_right);
                    else if (value == "left")
                        state->mouse.handedness(mir_pointer_handedness_left);
                    else
                        mir::log_warning(
                            "Config key '%s' has invalid value: %s",
                            key.to_string().c_str(),
                            val->data());
                }
            });

        config_handler.add_string_attribute({"touchpad", "scroll_mode"},
                                            [this](ConfigurationKey const& key, std::optional<std::string_view> val)
            {
                if (val)
                {
                    std::lock_guard lock{state->mutex};
                    auto const& value = *val;
                    if (value == "none")
                        state->touchpad.scroll_mode(mir_touchpad_scroll_mode_none);
                    else if (value == "two_finger_scroll")
                        state->touchpad.scroll_mode(mir_touchpad_scroll_mode_two_finger_scroll);
                    else if (value == "edge_scroll")
                        state->touchpad.scroll_mode(mir_touchpad_scroll_mode_edge_scroll);
                    else if (value == "button_down_scroll")
                        state->touchpad.scroll_mode(mir_touchpad_scroll_mode_button_down_scroll);
                    else
                        mir::log_warning(
                            "Config key '%s' has invalid value: %s",
                            key.to_string().c_str(),
                            val->data());
                }
            });

        config_handler.add_int_attribute({"repeat_rate"},
                                         [this](ConfigurationKey const& key, std::optional<int> val)
            {
                if (val)
                {
                    if (val >= 0)
                    {
                        std::lock_guard lock{state->mutex};
                        state->keyboard.set_repeat_rate(*val);
                    }
                    else
                    {
                        mir::log_warning(
                            "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                            key.to_string().c_str(), *val);
                    }
                }
            });

        config_handler.add_int_attribute({"repeat_delay"},
                                         [this](ConfigurationKey const& key, std::optional<int> val)
            {
                if (val)
                {
                    if (val >= 0)
                    {
                        std::lock_guard lock{state->mutex};
                        state->keyboard.set_repeat_delay(*val);
                    }
                    else
                    {
                        mir::log_warning(
                            "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                            key.to_string().c_str(), *val);
                    }
                }
            });

        config_handler.on_done([this]
        {
            std::lock_guard lock{state->mutex};
            mouse(state->mouse);
            touchpad(state->touchpad);
            keyboard(state->keyboard);
        });
    }

    void start_callback()
    {
        // Merge the input options collected from the command line,
        // environment, and default `.config` file with the options we
        // read from the `.input` file.
        //
        // In this case, the `.input` file takes precedence. You can
        // reverse the arguments to de-prioritize it.
        std::lock_guard lock{state->mutex};
        state->mouse.merge(mouse());
        state->keyboard.merge(keyboard());
        state->touchpad.merge(touchpad());
    }
};

// Struct for illustrative purposes, this would be rolled into miral::InputConfiguration
struct CursorScale : miral::CursorScale
{
    explicit CursorScale(AttributeHandler& config_handler) : miral::CursorScale{}
    {
        config_handler.add_float_attribute({"cursor", "scale"},
                                           [this](ConfigurationKey const& key, std::optional<float> val)
            {
                if (val)
                {
                    if (*val >= 0.0)
                    {
                        scale(*val);
                    }
                    else
                    {
                        mir::log_warning(
                            "Config value %s does not support negative values. Ignoring the supplied value (%f)...",
                            key.to_string().c_str(), *val);
                    }
                }
            });
    }
};

class DemoConfigFile : public AttributeHandler
{
public:
    DemoConfigFile(miral::MirRunner& runner, std::filesystem::path file) :
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

    void add_int_attribute(ConfigurationKey const& key, HandleInt handler) override;
    void add_bool_attribute(const ConfigurationKey& key, HandleBool handler) override;
    void add_float_attribute(ConfigurationKey const& key, HandleFloat handler) override;
    void add_string_attribute(ConfigurationKey const& key, HandleString handler) override;
    void on_done(HandleDone handler) override;

private:

    using MyLess = decltype([](ConfigurationKey const& l, ConfigurationKey const& r) { return l.to_string() < r.to_string(); });
    std::map<ConfigurationKey, HandleString, MyLess> attribute_handlers;
    std::list<HandleDone> done_handlers;

    std::mutex config_mutex;
    miral::ConfigFile config_file;

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
                auto const key = ConfigurationKey{line.substr(0, eq)};
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

void DemoConfigFile::on_done(HandleDone handler)
{
    std::lock_guard lock{config_mutex};
    done_handlers.emplace_back(std::move(handler));
}

void DemoConfigFile::add_int_attribute(ConfigurationKey const& key, HandleInt handler)
{
    add_string_attribute(key, [handler](ConfigurationKey const& key, std::optional<std::string_view> val)
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
                    val->data());
                    handler(key, std::nullopt);
            }
        }
        else
        {
            handler(key, std::nullopt);
        }
    });
}

void DemoConfigFile::add_bool_attribute(const ConfigurationKey& key, HandleBool handler)
{
    add_string_attribute(key, [handler](ConfigurationKey const& key, std::optional<std::string_view> val)
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
                    "Config key '%s' has invalid boolean value: %s",
                    key.to_string().c_str(),
                    val->data());
                handler(key, std::nullopt);
            }
        }
        else
        {
            handler(key, std::nullopt);
        }
    });
}

void DemoConfigFile::add_float_attribute(ConfigurationKey const& key, HandleFloat handler)
{
    add_string_attribute(key, [handler](ConfigurationKey const& key, std::optional<std::string_view> val)
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
                    "Config key '%s' has invalid floating point value: %s",
                    key.to_string().c_str(),
                    val->data());
                    handler(key, std::nullopt);
            }
        }
        else
        {
            handler(key, std::nullopt);
        }
    });
}

void DemoConfigFile::add_string_attribute(ConfigurationKey const& key, HandleString handler)
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

    CursorScale cursor_scale{demo_configuration};
    OutputFilter output_filter{demo_configuration};
    InputConfiguration input_configuration{demo_configuration};
    runner.add_start_callback([&input_configuration] { input_configuration.start_callback(); });

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
