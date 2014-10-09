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

struct mir::Server::BuildersAndWrappers
{
    std::function<std::shared_ptr<input::CursorListener>()> cursor_listener_builder;
    std::function<std::shared_ptr<scene::PlacementStrategy>()> placement_strategy_builder;
    std::function<std::shared_ptr<scene::SessionListener>()> session_listener_builder;
    std::function<std::shared_ptr<scene::PromptSessionListener>()> prompt_session_listener_builder;
    std::function<std::shared_ptr<scene::SurfaceConfigurator>()> surface_configurator_builder;
    std::function<std::shared_ptr<frontend::SessionAuthorizer>()> session_authorizer_builder;
    std::function<std::shared_ptr<compositor::Compositor>()> compositor_builder;
    std::function<std::shared_ptr<input::InputDispatcher>()> input_dispatcher_builder;
    std::function<std::shared_ptr<graphics::GLConfig>()> gl_config_builder;
    std::function<std::shared_ptr<ServerStatusListener>()> server_status_listener_builder;
    std::function<std::shared_ptr<shell::FocusSetter>()> shell_focus_setter_builder;

    std::function<std::shared_ptr<scene::SessionCoordinator>(std::shared_ptr<scene::SessionCoordinator>)> session_coordinator_wrapper;
    std::function<std::shared_ptr<scene::SurfaceCoordinator>(std::shared_ptr<scene::SurfaceCoordinator>)> surface_coordinator_wrapper;
};

#define MIR_SERVER_CONFIG_OVERRIDE(name)\
auto the_##name()\
-> decltype(mir::DefaultServerConfiguration::the_##name()) override\
{\
    if (builders_and_wrappers->name##_builder)\
        return name(\
            [this] { return builders_and_wrappers->name##_builder(); });\
\
    return mir::DefaultServerConfiguration::the_##name();\
}

#define MIR_SERVER_CONFIG_WRAP(name)\
auto wrap_##name(decltype(BuildersAndWrappers::name##_wrapper)::result_type const& wrapped)\
-> decltype(mir::DefaultServerConfiguration::wrap_##name({})) override\
{\
    if (builders_and_wrappers->name##_wrapper)\
        return name(\
            [&] { return builders_and_wrappers->name##_wrapper(wrapped); });\
\
    return mir::DefaultServerConfiguration::wrap_##name(wrapped);\
}

struct mir::Server::ServerConfiguration : mir::DefaultServerConfiguration
{
    ServerConfiguration(
        std::shared_ptr<options::Configuration> const& configuration_options,
        std::shared_ptr<BuildersAndWrappers> const builders_and_wrappers) :
        DefaultServerConfiguration(configuration_options),
        builders_and_wrappers(builders_and_wrappers)
    {
    }

    using mir::DefaultServerConfiguration::the_options;

    // TODO the MIR_SERVER_CONFIG_OVERRIDE macro expects a CachePtr named
    // TODO "placement_strategy" not "shell_placement_strategy".
    // Unfortunately, "shell_placement_strategy" is currently part of our
    // published API and used by qtmir: we cannot just rename it to remove
    // this ugliness. (Yet.)
    decltype(shell_placement_strategy)& placement_strategy = shell_placement_strategy;

    MIR_SERVER_CONFIG_OVERRIDE(cursor_listener)
    MIR_SERVER_CONFIG_OVERRIDE(placement_strategy)
    MIR_SERVER_CONFIG_OVERRIDE(session_listener)
    MIR_SERVER_CONFIG_OVERRIDE(prompt_session_listener)
    MIR_SERVER_CONFIG_OVERRIDE(surface_configurator)
    MIR_SERVER_CONFIG_OVERRIDE(session_authorizer)
    MIR_SERVER_CONFIG_OVERRIDE(compositor)
    MIR_SERVER_CONFIG_OVERRIDE(input_dispatcher)
    MIR_SERVER_CONFIG_OVERRIDE(gl_config)
    MIR_SERVER_CONFIG_OVERRIDE(server_status_listener)
    MIR_SERVER_CONFIG_OVERRIDE(shell_focus_setter)

    MIR_SERVER_CONFIG_WRAP(session_coordinator)
    MIR_SERVER_CONFIG_WRAP(surface_coordinator)

    std::shared_ptr<BuildersAndWrappers> const builders_and_wrappers;
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
}

mir::Server::Server() :
    builders_and_wrappers(std::make_shared<BuildersAndWrappers>())
{
}

void mir::Server::set_add_configuration_options(
    std::function<void(mo::DefaultConfiguration& config)> const& add_configuration_options)
{
    this->add_configuration_options = add_configuration_options;
}


void mir::Server::set_command_line(int argc, char const* argv[])
{
    this->argc = argc;
    this->argv = argv;
}

void mir::Server::set_init_callback(std::function<void()> const& init_callback)
{
    this->init_callback = init_callback;
}

auto mir::Server::get_options() const -> std::shared_ptr<options::Option>
{
    return options.lock();
}

void mir::Server::set_exception_handler(std::function<void()> const& exception_handler)
{
    this->exception_handler = exception_handler;
}

void mir::Server::run()
try
{
    auto const options = configuration_options(argc, argv, command_line_hander);

    add_configuration_options(*options);

    ServerConfiguration config{options, builders_and_wrappers};

    server_config = &config;

    run_mir(config, [&](DisplayServer&)
        {
            this->options = config.the_options();
            init_callback();
        });

    exit_status = true;
    server_config = nullptr;
}
catch (...)
{
    server_config = nullptr;

    if (exception_handler)
        exception_handler();
    else
        mir::report_exception(std::cerr);
}

void mir::Server::stop()
{
    std::cerr << "DEBUG: " << __PRETTY_FUNCTION__ << std::endl;
    if (auto const main_loop = the_main_loop())
        main_loop->stop();
}

bool mir::Server::exited_normally()
{
    return exit_status;
}

namespace
{
auto const no_config_to_access = "Cannot access config when no config active.";
}

#define MIR_SERVER_ACCESSOR(name)\
auto mir::Server::name() const -> decltype(server_config->name())\
{\
    if (server_config) return server_config->name();\
    BOOST_THROW_EXCEPTION(std::logic_error(no_config_to_access));\
}

MIR_SERVER_ACCESSOR(the_graphics_platform)
MIR_SERVER_ACCESSOR(the_display)
MIR_SERVER_ACCESSOR(the_main_loop)
MIR_SERVER_ACCESSOR(the_composite_event_filter)
MIR_SERVER_ACCESSOR(the_shell_display_layout)
MIR_SERVER_ACCESSOR(the_session_authorizer)
MIR_SERVER_ACCESSOR(the_session_listener)
MIR_SERVER_ACCESSOR(the_prompt_session_listener)
MIR_SERVER_ACCESSOR(the_surface_configurator)

#undef MIR_SERVER_ACCESSOR

#define MIR_SERVER_OVERRIDE(name)\
void mir::Server::override_the_##name(decltype(BuildersAndWrappers::name##_builder) const& value)\
{\
    builders_and_wrappers->name##_builder = value;\
}

MIR_SERVER_OVERRIDE(cursor_listener)
MIR_SERVER_OVERRIDE(placement_strategy)
MIR_SERVER_OVERRIDE(session_listener)
MIR_SERVER_OVERRIDE(prompt_session_listener)
MIR_SERVER_OVERRIDE(surface_configurator)
MIR_SERVER_OVERRIDE(session_authorizer)
MIR_SERVER_OVERRIDE(compositor)
MIR_SERVER_OVERRIDE(input_dispatcher)
MIR_SERVER_OVERRIDE(gl_config)
MIR_SERVER_OVERRIDE(server_status_listener)
MIR_SERVER_OVERRIDE(shell_focus_setter)

#undef MIR_SERVER_OVERRIDE

#define MIR_SERVER_WRAP(name)\
void mir::Server::wrap_##name(decltype(BuildersAndWrappers::name##_wrapper) const& value)\
{\
    builders_and_wrappers->name##_wrapper = value;\
}

MIR_SERVER_WRAP(session_coordinator)
MIR_SERVER_WRAP(surface_coordinator)

#undef MIR_SERVER_WRAP
