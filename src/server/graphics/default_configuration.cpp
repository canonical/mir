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

#include "default_display_configuration_policy.h"
#include "nested/nested_platform.h"
#include "mir/graphics/buffer_initializer.h"

#include "mir/input/nested_input_relay.h"
#include "mir/shared_library.h"

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
            auto graphics_lib = load_library(the_options()->get(platform_graphics_lib, default_platform_graphics_lib));

            // TODO (default-nested): don't fallback to standalone if host socket is unset in 14.04
            if (the_options()->is_set(standalone_opt) || !the_options()->is_set(host_socket_opt))
            {
                auto create_platform = graphics_lib->load_function<mg::CreatePlatform>("create_platform");
                return create_platform(the_options(), the_display_report());
            }

            auto create_native_platform = graphics_lib->load_function<mg::CreateNativePlatform>("create_native_platform");

            return std::make_shared<mir::graphics::nested::NestedPlatform>(
                the_host_connection(),
                the_nested_input_relay(),
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
        [this]()
        {
            return the_graphics_platform()->create_display(
                the_display_configuration_policy());
        });
}
