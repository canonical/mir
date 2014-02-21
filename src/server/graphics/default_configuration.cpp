/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"

#include "default_display_configuration_policy.h"
#include "nested/host_connection.h"
#include "nested/nested_platform.h"
#include "offscreen/display.h"

#include "mir/graphics/buffer_initializer.h"

#include "mir/shared_library.h"
#include "mir/abnormal_exit.h"

#include <boost/throw_exception.hpp>

#include <map>

namespace mg = mir::graphics;

namespace
{
mir::SharedLibrary const* load_library(std::string const& libname)
{
    // There's no point in loading twice, and it isn't safe to unload...
    static std::map<std::string, std::shared_ptr<mir::SharedLibrary>> libraries_cache;

    if (auto& ptr = libraries_cache[libname])
    {
        return ptr.get();
    }
    else
    {
        ptr = std::make_shared<mir::SharedLibrary>(libname);
        return ptr.get();
    }
}
}

std::shared_ptr<mg::BufferInitializer>
mir::DefaultServerConfiguration::the_buffer_initializer()
{
    return buffer_initializer(
        []()
        {
             return std::make_shared<mg::NullBufferInitializer>();
        });
}

std::shared_ptr<mg::DisplayConfigurationPolicy>
mir::DefaultServerConfiguration::the_display_configuration_policy()
{
    return display_configuration_policy(
        []
        {
            return std::make_shared<mg::DefaultDisplayConfigurationPolicy>();
        });
}

std::shared_ptr<mg::Platform> mir::DefaultServerConfiguration::the_graphics_platform()
{
    return graphics_platform(
        [this]()->std::shared_ptr<mg::Platform>
        {
            auto graphics_lib = load_library(the_options()->get<std::string>(options::platform_graphics_lib));

            // TODO (default-nested): don't fallback to standalone if host socket is unset in 14.04
            if (the_options()->is_set(options::standalone_opt) || !the_options()->is_set(options::host_socket_opt))
            {
                auto create_platform = graphics_lib->load_function<mg::CreatePlatform>("create_platform");
                return create_platform(the_options(), the_display_report());
            }

            auto create_native_platform = graphics_lib->load_function<mg::CreateNativePlatform>("create_native_platform");

            return std::make_shared<mir::graphics::nested::NestedPlatform>(
                the_host_connection(),
                the_nested_event_filter(),
                the_display_report(),
                create_native_platform(the_display_report()));
        });
}

std::shared_ptr<mg::GraphicBufferAllocator>
mir::DefaultServerConfiguration::the_buffer_allocator()
{
    return buffer_allocator(
        [&]()
        {
            return the_graphics_platform()->create_buffer_allocator(the_buffer_initializer());
        });
}

std::shared_ptr<mg::Display>
mir::DefaultServerConfiguration::the_display()
{
    return display(
        [this]() -> std::shared_ptr<mg::Display>
        {
            if (the_options()->is_set(options::offscreen_opt))
            {
                return std::make_shared<mg::offscreen::Display>(
                    the_graphics_platform(),
                    the_display_configuration_policy(),
                    the_display_report());
            }
            else
            {
                return the_graphics_platform()->create_display(
                    the_display_configuration_policy());
            }
        });
}

auto mir::DefaultServerConfiguration::the_host_connection()
-> std::shared_ptr<graphics::nested::HostConnection>
{
    return host_connection(
        [this]() -> std::shared_ptr<graphics::nested::HostConnection>
        {
            auto const options = the_options();

            if (!options->is_set(options::standalone_opt))
            {
                if (!options->is_set(options::host_socket_opt))
                    BOOST_THROW_EXCEPTION(mir::AbnormalExit("Exiting Mir! Specify either $MIR_SOCKET or --standalone"));

                auto host_socket = options->get<std::string>(options::host_socket_opt);
                auto server_socket = the_socket_file();

                if (server_socket == host_socket)
                    BOOST_THROW_EXCEPTION(mir::AbnormalExit("Exiting Mir! Reason: Nested Mir and Host Mir cannot use the same socket file to accept connections!"));

                auto const my_name = options->is_set(options::name_opt) ?
                    options->get<std::string>(options::name_opt) :
                    "nested-mir@:" + server_socket;

                return std::make_shared<graphics::nested::HostConnection>(
                    host_socket,
                    my_name);
            }
            else
            {
                BOOST_THROW_EXCEPTION(std::logic_error("can only use host connection in nested mode"));
            }
        });
}
