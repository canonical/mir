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

struct mir::Server::DefaultServerConfiguration : mir::DefaultServerConfiguration
{
    using mir::DefaultServerConfiguration::DefaultServerConfiguration;
    using mir::DefaultServerConfiguration::the_options;

    std::function<std::shared_ptr<scene::PlacementStrategy>()> placement_strategy_builder;

    auto the_placement_strategy()
    -> std::shared_ptr<scene::PlacementStrategy> override
    {
        if (placement_strategy_builder)
            return shell_placement_strategy(
                [this] { return placement_strategy_builder(); });

        return mir::DefaultServerConfiguration::the_placement_strategy();
    }
};

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

    DefaultServerConfiguration config{options};

    config.placement_strategy_builder = placement_strategy_builder;

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

auto mir::Server::the_graphics_platform() const -> std::shared_ptr<graphics::Platform>
{
    if (server_config) return server_config->the_graphics_platform();

    return {};
}

auto mir::Server::the_display() const -> std::shared_ptr<graphics::Display>
{
    if (server_config) return server_config->the_display();

    return {};
}

auto mir::Server::the_main_loop() const -> std::shared_ptr<MainLoop>
{
    if (server_config) return server_config->the_main_loop();

    return {};
}

auto mir::Server::the_composite_event_filter() const
-> std::shared_ptr<input::CompositeEventFilter>
{
    if (server_config) return server_config->the_composite_event_filter();

    return {};
}

auto mir::Server::the_shell_display_layout() const
-> std::shared_ptr<shell::DisplayLayout>
{
    if (server_config) return server_config->the_shell_display_layout();

    return {};
}

auto mir::Server::the_session_authorizer() const
-> std::shared_ptr<frontend::SessionAuthorizer>
{
    if (server_config) return server_config->the_session_authorizer();

    return {};
}

auto mir::Server::the_session_listener() const
-> std::shared_ptr<scene::SessionListener>
{
    if (server_config) return server_config->the_session_listener();

    return {};
}

auto mir::Server::the_prompt_session_listener() const
-> std::shared_ptr<scene::PromptSessionListener>
{
    if (server_config) return server_config->the_prompt_session_listener();

    return {};
}

auto mir::Server::the_surface_configurator() const
-> std::shared_ptr<scene::SurfaceConfigurator>
{
    if (server_config) return server_config->the_surface_configurator();

    return {};
}

void mir::Server::override_the_placement_strategy(std::function<std::shared_ptr<scene::PlacementStrategy>()> const& placement_strategy_builder)
{
    this->placement_strategy_builder = placement_strategy_builder;
}
