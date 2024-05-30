/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/default_server_configuration.h"
#include "mir/options/configuration.h"

#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "multiplexing_display.h"
#include "null_cursor.h"
#include "software_cursor.h"
#include "platform_probe.h"

#include "mir/graphics/gl_config.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/cursor.h"
#include "display_configuration_observer_multiplexer.h"

#include "mir/shared_library.h"
#include "mir/shared_library_prober.h"
#include "mir/abnormal_exit.h"
#include "mir/emergency_cleanup.h"
#include "mir/log.h"
#include "mir/report_exception.h"
#include "mir/main_loop.h"
#include "mir/udev/wrapper.h"

#include <algorithm>
#include <boost/throw_exception.hpp>

#include <sstream>

namespace mg = mir::graphics;

std::shared_ptr<mg::DisplayConfigurationPolicy>
mir::DefaultServerConfiguration::the_display_configuration_policy()
{
    return display_configuration_policy(
        [this]
        {
            return wrap_display_configuration_policy(
                std::make_shared<mg::CloneDisplayConfigurationPolicy>());
        });
}

std::shared_ptr<mg::DisplayConfigurationPolicy>
mir::DefaultServerConfiguration::wrap_display_configuration_policy(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
{
    return wrapped;
}

namespace
{
auto split_on(std::string const& tokens, char delimiter) -> std::vector<std::string>
{
    std::string token;
    std::istringstream token_stream{tokens};

    std::vector<std::string> result;
    while (std::getline(token_stream, token, delimiter))
    {
        result.push_back(token);
    }

    return result;
}

auto select_platforms_from_list(std::string const& selection, std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules)
    -> std::vector<std::shared_ptr<mir::SharedLibrary>>
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> selected_modules;
    std::vector<std::string> found_module_names;

    // Our platform modules are a comma-delimited list.
    auto requested_modules = split_on(selection, ',');

    for (auto const& module : modules)
    {
        try
        {
            auto describe_module = module->load_function<mg::DescribeModule>(
                "describe_graphics_module",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            auto const description = describe_module();
            found_module_names.emplace_back(description->name);

            if (auto const i = std::find(requested_modules.begin(), requested_modules.end(), description->name);
                i != requested_modules.end())
            {
                selected_modules.push_back(module);
                requested_modules.erase(i);
            }
        }
        catch (std::exception const&)
        {
            // Should we log anything here?
        }
    }

    if (!requested_modules.empty())
    {
        std::stringstream error_msg;
        error_msg << "Failed to find all requested platform modules." << std::endl;
        error_msg << "Detected modules are: " << std::endl;
        for (auto const& module : found_module_names)
        {
            error_msg << "\t" << module << std::endl;
        }
        error_msg << "Failed to find:" << std::endl;
        for (auto const& module : requested_modules)
        {
            error_msg << "\t" << module << std::endl;
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{error_msg.str()}));
    }

    return selected_modules;
}
}

auto mir::DefaultServerConfiguration::the_display_platforms() -> std::vector<std::shared_ptr<graphics::DisplayPlatform>> const&
{
    if (display_platforms.empty())
    {
        std::stringstream error_report;

        try
        {
            auto const platform_modules = mg::select_display_modules(*the_options(), the_console_services(), *the_shared_library_prober_report());

            for (auto const& [device, platform]: platform_modules)
            {
                auto create_display_platform = platform->load_function<mg::CreateDisplayPlatform>(
                    "create_display_platform",
                    MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

                auto describe_module = platform->load_function<mg::DescribeModule>(
                    "describe_graphics_module",
                    MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                
                auto description = describe_module();
                if (!device.device)
                {
                    mir::log_info(
                        "Selected display driver: %s (version %d.%d.%d) for platform",
                        description->name,
                        description->major_version,
                        description->minor_version,
                        description->micro_version);
                }
                else
                {
                    mir::log_info(
                        "Selected display driver: %s (version %d.%d.%d) for device (%s: %s)",
                        description->name,
                        description->major_version,
                        description->minor_version,
                        description->micro_version,
                        device.device->driver(),
                        device.device->devnode());
                }

                // TODO: Do we want to be able to continue on partial failure here?
                display_platforms.push_back(
                    create_display_platform(
                        device,
                        the_options(),
                        the_emergency_cleanup(),
                        the_console_services(),
                        the_display_report()));
                // Add this module to the list searched by the input stack later
                // TODO: Come up with a more principled solution for combined input/rendering/output platforms
                platform_libraries.push_back(platform);
            }
        }
        catch(std::exception const&)
        {
            // access exception information before platform library gets unloaded
            error_report << "Exception while creating graphics platform" << std::endl;
            mir::report_exception(error_report);
        }
        if (display_platforms.empty())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(error_report.str()));
        }
    }

    return display_platforms;
}

auto mir::DefaultServerConfiguration::the_rendering_platforms() ->
    std::vector<std::shared_ptr<graphics::RenderingPlatform>> const&
{
    if (rendering_platforms.empty())
    {
        std::stringstream error_report;
        std::vector<std::pair<mg::SupportedDevice, std::shared_ptr<mir::SharedLibrary>>> platform_modules;
        std::multimap<std::string, std::shared_ptr<graphics::RenderingPlatform>> rendering_platform_map;

        try
        {
            auto const& path = the_options()->get<std::string>(options::platform_path);
            auto platforms = mir::libraries_for_path(path, *the_shared_library_prober_report());

            if (platforms.empty())
            {
                auto msg = "Failed to find any platform plugins in: " + path;
                throw std::runtime_error(msg.c_str());
            }

            auto display_targets = the_display_platforms();

            if (the_options()->is_set(options::platform_rendering_libs))
            {
                auto const manually_selected_platforms =
                    select_platforms_from_list(the_options()->get<std::string>(options::platform_rendering_libs), platforms);

                for (auto const& platform : manually_selected_platforms)
                {
                    auto supported_devices =
                        graphics::probe_rendering_module(
                            display_targets,
                            *platform,
                            dynamic_cast<mir::options::ProgramOption&>(*the_options()),
                            the_console_services());

                    bool found_supported_device{false};
                    for (auto& device : supported_devices)
                    {
                        if (device.support_level >= mg::probe::supported)
                        {
                            found_supported_device = true;
                        }
                        platform_modules.emplace_back(std::move(device), platform);
                    }

                    if (!found_supported_device)
                    {
                        auto const describe_module = platform->load_function<mg::DescribeModule>(
                            "describe_graphics_module",
                            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                        auto const descriptor = describe_module();

                        mir::log_warning("Manually-specified rendering platform %s does not claim to support this system. Trying anyway...", descriptor->name);
                    }
                }
            }
            else
            {
                platform_modules = mir::graphics::rendering_modules_for_device(platforms, display_targets, dynamic_cast<mir::options::ProgramOption&>(*the_options()), the_console_services());
            }

            for (auto const& [device, platform]: platform_modules)
            {
                auto create_rendering_platform = platform->load_function<mg::CreateRenderPlatform>(
                    "create_rendering_platform",
                    MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

                auto describe_module = platform->load_function<mg::DescribeModule>(
                    "describe_graphics_module",
                    MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

                auto description = describe_module();
                if (!device.device)
                {
                    mir::log_info(
                        "Selected rendering driver: %s (version %d.%d.%d) for platform",
                        description->name,
                        description->major_version,
                        description->minor_version,
                        description->micro_version);
                }
                else
                {
                    mir::log_info(
                        "Selected rendering driver: %s (version %d.%d.%d) for device (%s: %s)",
                        description->name,
                        description->major_version,
                        description->minor_version,
                        description->micro_version,
                        device.device->driver(),
                        device.device->devnode());
                }

                // TODO: Do we want to be able to continue on partial failure here?
                rendering_platform_map.emplace(
                    description->name,
                    create_rendering_platform(
                        device,
                        display_targets,
                        *the_options(),
                        *the_emergency_cleanup()));
                // Add this module to the list searched by the input stack later
                // TODO: Come up with a more principled solution for combined input/rendering/output platforms
                platform_libraries.push_back(platform);
            }
        }
        catch(std::exception const&)
        {
            // access exception information before platform library gets unloaded
            error_report << "Exception while creating rendering platform" << std::endl;
            mir::report_exception(error_report);
        }

        rendering_platforms.reserve(rendering_platform_map.size());

        {
            // We want to make the egl-generic platform the last resort
            // So don't push it into rendering_platforms until the rest are done
            auto const [egl_generic_begin, egl_generic_end] = rendering_platform_map.equal_range("mir:egl-generic");

            for (auto rp = rendering_platform_map.begin(); rp != egl_generic_begin; ++rp)
            {
                rendering_platforms.push_back(rp->second);
            }
            for (auto rp = egl_generic_end; rp != rendering_platform_map.end(); ++rp)
            {
                rendering_platforms.push_back(rp->second);
            }
            for (auto rp = egl_generic_begin; rp != egl_generic_end; ++rp)
            {
                rendering_platforms.push_back(rp->second);
            }
        }

        if (rendering_platforms.empty())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(error_report.str()));
        }
    }

    return rendering_platforms;
}

auto mir::DefaultServerConfiguration::the_platform_libaries()
    -> std::vector<std::shared_ptr<mir::SharedLibrary>> const&
{
    return platform_libraries;
}

std::shared_ptr<mg::GraphicBufferAllocator>
mir::DefaultServerConfiguration::the_buffer_allocator()
{
    return buffer_allocator(
        [&]() -> std::shared_ptr<graphics::GraphicBufferAllocator>
        {
            auto rendering_platforms = the_rendering_platforms();
            auto best_provider = graphics::select_buffer_allocating_renderer(
                *the_display(),
                rendering_platforms);
            // TODO: More than one BufferAllocator
            return best_provider->create_buffer_allocator(*the_display());
        });
}

std::shared_ptr<mg::Display>
mir::DefaultServerConfiguration::the_display()
{
    return display(
        [this]() -> std::shared_ptr<mg::Display>
        {
            std::vector<std::unique_ptr<mg::Display>> displays;
            displays.reserve(the_display_platforms().size());
            for (auto const& platform : the_display_platforms())
            {
                displays.push_back(
                    platform->create_display(
                        the_display_configuration_policy(),
                        the_gl_config()));
            }
            return std::make_shared<mg::MultiplexingDisplay>(
                std::move(displays),
                *the_display_configuration_policy());
        });
}

std::shared_ptr<mg::Cursor>
mir::DefaultServerConfiguration::the_cursor()
{
    return cursor(
        [this]() -> std::shared_ptr<mg::Cursor>
        {
            std::shared_ptr<mg::Cursor> primary_cursor;

            auto cursor_choice = the_options()->get<std::string>(options::cursor_opt);

            if (cursor_choice == "null")
            {
                mir::log_info("Cursor disabled");
                primary_cursor = std::make_shared<mg::NullCursor>();
            }
            else if (cursor_choice != "software" &&
                     (primary_cursor = the_display()->create_hardware_cursor()))
            {
                mir::log_info("Using hardware cursor");
            }
            else
            {
                mir::log_info("Using software cursor");
                primary_cursor = std::make_shared<mg::SoftwareCursor>(
                    the_buffer_allocator(),
                    the_main_loop(),
                    the_input_scene());
            }

            primary_cursor->show(*the_default_cursor_image());
            return wrap_cursor(primary_cursor);
        });
}

std::shared_ptr<mg::Cursor>
mir::DefaultServerConfiguration::wrap_cursor(std::shared_ptr<mg::Cursor> const& wrapped)
{
    return wrapped;
}

std::shared_ptr<mg::GLConfig>
mir::DefaultServerConfiguration::the_gl_config()
{
    return gl_config(
        []
        {
            struct NoGLConfig : public mg::GLConfig
            {
                int depth_buffer_bits() const override { return 0; }
                int stencil_buffer_bits() const override { return 0; }
            };
            return std::make_shared<NoGLConfig>();
        });
}

std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>>
mir::DefaultServerConfiguration::the_display_configuration_observer_registrar()
{
    return display_configuration_observer_multiplexer(
        [default_executor = the_main_loop()]
        {
            return std::make_shared<mg::DisplayConfigurationObserverMultiplexer>(default_executor);
        });
}

std::shared_ptr<mg::DisplayConfigurationObserver>
mir::DefaultServerConfiguration::the_display_configuration_observer()
{
    return display_configuration_observer_multiplexer(
        [default_executor = the_main_loop()]
        {
            return std::make_shared<mg::DisplayConfigurationObserverMultiplexer>(default_executor);
        });
}
