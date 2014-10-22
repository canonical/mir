/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/server.h"

#include "mir/options/default_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir/main_loop.h"
#include "mir/report_exception.h"
#include "mir/run_mir.h"

#include <iostream>

namespace mo = mir::options;

#define FOREACH_WRAPPER(MACRO)\
    MACRO(display_configuration_policy)\
    MACRO(session_coordinator)\
    MACRO(surface_coordinator)

#define FOREACH_OVERRIDE(MACRO)\
    MACRO(compositor)\
    MACRO(cursor_listener)\
    MACRO(gl_config)\
    MACRO(input_dispatcher)\
    MACRO(placement_strategy)\
    MACRO(prompt_session_listener)\
    MACRO(server_status_listener)\
    MACRO(session_authorizer)\
    MACRO(session_listener)\
    MACRO(shell_focus_setter)\
    MACRO(surface_configurator)

#define FOREACH_ACCESSOR(MACRO)\
    MACRO(the_composite_event_filter)\
    MACRO(the_display)\
    MACRO(the_graphics_platform)\
    MACRO(the_main_loop)\
    MACRO(the_prompt_session_listener)\
    MACRO(the_session_authorizer)\
    MACRO(the_session_coordinator)\
    MACRO(the_session_listener)\
    MACRO(the_shell_display_layout)\
    MACRO(the_surface_configurator)\
    MACRO(the_surface_coordinator)

#define MIR_SERVER_BUILDER(name)\
    std::function<std::result_of<decltype(&mir::DefaultServerConfiguration::the_##name)(mir::DefaultServerConfiguration*)>::type()> name##_builder;

#define MIR_SERVER_WRAPPER(name)\
    std::function<std::result_of<decltype(&mir::DefaultServerConfiguration::the_##name)(mir::DefaultServerConfiguration*)>::type\
        (std::result_of<decltype(&mir::DefaultServerConfiguration::the_##name)(mir::DefaultServerConfiguration*)>::type const&)> name##_wrapper;

struct mir::Server::Self
{
    bool exit_status{false};
    std::weak_ptr<options::Option> options;
    std::shared_ptr<ServerConfiguration> server_config;

    std::function<void()> init_callback{[]{}};
    int argc{0};
    char const** argv{nullptr};
    std::function<void()> exception_handler{};

    std::function<void(int argc, char const* const* argv)> command_line_hander{};

    /// set a callback to introduce additional configuration options.
    /// this will be invoked by run() before server initialisation starts
    void set_add_configuration_options(
        std::function<void(options::DefaultConfiguration& config)> const& add_configuration_options);

    std::function<void(options::DefaultConfiguration& config)> add_configuration_options{
        [](options::DefaultConfiguration&){}};

    FOREACH_OVERRIDE(MIR_SERVER_BUILDER)

    FOREACH_WRAPPER(MIR_SERVER_WRAPPER)
};

#undef MIR_SERVER_BUILDER
#undef MIR_SERVER_WRAPPER

#define MIR_SERVER_CONFIG_OVERRIDE(name)\
auto the_##name()\
-> decltype(mir::DefaultServerConfiguration::the_##name()) override\
{\
    if (self->name##_builder)\
        return name(\
            [this] { return self->name##_builder(); });\
\
    return mir::DefaultServerConfiguration::the_##name();\
}

#define MIR_SERVER_CONFIG_WRAP(name)\
auto wrap_##name(decltype(Self::name##_wrapper)::result_type const& wrapped)\
-> decltype(mir::DefaultServerConfiguration::wrap_##name({})) override\
{\
    if (self->name##_wrapper)\
        return name(\
            [&] { return self->name##_wrapper(wrapped); });\
\
    return mir::DefaultServerConfiguration::wrap_##name(wrapped);\
}

struct mir::Server::ServerConfiguration : mir::DefaultServerConfiguration
{
    ServerConfiguration(
        std::shared_ptr<options::Configuration> const& configuration_options,
        std::shared_ptr<Self> const& self) :
        DefaultServerConfiguration(configuration_options),
        self(self.get())
    {
    }

    using mir::DefaultServerConfiguration::the_options;

    // TODO the MIR_SERVER_CONFIG_OVERRIDE macro expects a CachePtr named
    // TODO "placement_strategy" not "shell_placement_strategy".
    // Unfortunately, "shell_placement_strategy" is currently part of our
    // published API and used by qtmir: we cannot just rename it to remove
    // this ugliness. (Yet.)
    decltype(shell_placement_strategy)& placement_strategy = shell_placement_strategy;

    FOREACH_OVERRIDE(MIR_SERVER_CONFIG_OVERRIDE)

    FOREACH_WRAPPER(MIR_SERVER_CONFIG_WRAP)

    Self* const self;
};

#undef MIR_SERVER_CONFIG_OVERRIDE
#undef MIR_SERVER_CONFIG_WRAP

namespace
{
std::shared_ptr<mo::DefaultConfiguration> configuration_options(
    int argc,
    char const** argv,
    std::function<void(int argc, char const* const* argv)> const& command_line_hander)
{
    if (command_line_hander)
        return std::make_shared<mo::DefaultConfiguration>(argc, argv, command_line_hander);
    else
        return std::make_shared<mo::DefaultConfiguration>(argc, argv);

}

template<typename ConfigPtr>
void verify_setting_allowed(ConfigPtr const& initialized)
{
    if (initialized)
       BOOST_THROW_EXCEPTION(std::logic_error("Cannot amend configuration after initialization starts"));
}
}

mir::Server::Server() :
    self(std::make_shared<Self>())
{
}

void mir::Server::Self::set_add_configuration_options(
    std::function<void(mo::DefaultConfiguration& config)> const& add_configuration_options)
{
    this->add_configuration_options = add_configuration_options;
}


void mir::Server::set_command_line(int argc, char const* argv[])
{
    verify_setting_allowed(self->server_config);    
    self->argc = argc;
    self->argv = argv;
}

void mir::Server::add_init_callback(std::function<void()> const& init_callback)
{
    verify_setting_allowed(self->server_config);    
    auto const& existing = self->init_callback;

    auto const updated = [=]
        {
            existing();
            init_callback();
        };

    self->init_callback = updated;
}

auto mir::Server::get_options() const -> std::shared_ptr<options::Option>
{
    return self->options.lock();
}

void mir::Server::set_exception_handler(std::function<void()> const& exception_handler)
{
    verify_setting_allowed(self->server_config);    
    self->exception_handler = exception_handler;
}

void mir::Server::apply_settings() const
{
    if (self->server_config) return;

    auto const options = configuration_options(self->argc, self->argv, self->command_line_hander);
    self->add_configuration_options(*options);

    auto const config = std::make_shared<ServerConfiguration>(options, self);
    self->server_config = config;
    self->options = config->the_options();
}

void mir::Server::run()
try
{
    apply_settings();

    run_mir(*self->server_config, [&](DisplayServer&) { self->init_callback(); });

    self->exit_status = true;
    self->server_config.reset();
}
catch (...)
{
    self->server_config = nullptr;

    if (self->exception_handler)
        self->exception_handler();
    else
        mir::report_exception(std::cerr);
}

void mir::Server::stop()
{
    if (auto const main_loop = the_main_loop())
        main_loop->stop();
}

bool mir::Server::exited_normally()
{
    return self->exit_status;
}

#define MIR_SERVER_ACCESSOR(name)\
auto mir::Server::name() const -> decltype(self->server_config->name())\
{\
    apply_settings();\
    return self->server_config->name();\
}

FOREACH_ACCESSOR(MIR_SERVER_ACCESSOR)

#undef MIR_SERVER_ACCESSOR

#define MIR_SERVER_OVERRIDE(name)\
void mir::Server::override_the_##name(decltype(Self::name##_builder) const& value)\
{\
    verify_setting_allowed(self->server_config);\
    self->name##_builder = value;\
}

FOREACH_OVERRIDE(MIR_SERVER_OVERRIDE)

#undef MIR_SERVER_OVERRIDE

#define MIR_SERVER_WRAP(name)\
void mir::Server::wrap_##name(decltype(Self::name##_wrapper) const& value)\
{\
    verify_setting_allowed(self->server_config);\
    self->name##_wrapper = value;\
}

FOREACH_WRAPPER(MIR_SERVER_WRAP)

#undef MIR_SERVER_WRAP

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    int default_)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    auto const option_adder = [=](options::DefaultConfiguration& config)
        {
            existing(config);

            config.add_options()
            (option.c_str(), po::value<int>()->default_value(default_), description.c_str());
        };

    self->set_add_configuration_options(option_adder);
}

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    std::string const& default_)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    auto const option_adder = [=](options::DefaultConfiguration& config)
        {
            existing(config);

            config.add_options()
            (option.c_str(), po::value<std::string>()->default_value(default_), description.c_str());
        };

    self->set_add_configuration_options(option_adder);
}

void mir::Server::add_configuration_option(
    std::string const& option,
    std::string const& description,
    OptionType type)
{
    verify_setting_allowed(self->server_config);
    namespace po = boost::program_options;

    auto const& existing = self->add_configuration_options;

    switch (type)
    {
    case OptionType::null:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;

    case OptionType::integer:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), po::value<int>(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;

    case OptionType::string:
        {
            auto const option_adder = [=](options::DefaultConfiguration& config)
                {
                    existing(config);

                    config.add_options()
                    (option.c_str(), po::value<std::string>(), description.c_str());
                };

            self->set_add_configuration_options(option_adder);
        }
        break;
    }
}
