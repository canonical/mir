/*
 * Copyright © 2013-2020 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"

#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/renderer/gl/egl_platform.h"
#include "null_cursor.h"
#include "offscreen/display.h"
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
#include "mir/main_loop.h"
#include "mir/report_exception.h"

#include "mir_toolkit/common.h"

#include <boost/throw_exception.hpp>

#include <map>
#include <sstream>

namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace mgn = mir::graphics::nested;

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

std::shared_ptr<mg::Platform> mir::DefaultServerConfiguration::the_graphics_platform()
{
    return graphics_platform(
        [this]()->std::shared_ptr<mg::Platform>
        {
            std::shared_ptr<mir::SharedLibrary> platform_library;
            std::stringstream error_report;
            try
            {
                // fallback to standalone if host socket is unset
                if (the_options()->is_set(options::platform_graphics_lib))
                {
                    platform_library = std::make_shared<mir::SharedLibrary>(the_options()->get<std::string>(options::platform_graphics_lib));
                    auto const platform_priority =
                        graphics::probe_module(
                            *platform_library,
                            dynamic_cast<mir::options::ProgramOption&>(*the_options()),
                            the_console_services());

                    if (platform_priority < mir::graphics::PlatformPriority::supported)
                    {
                        mir::log_warning("Manually-specified graphics platform does not claim to support this system. Trying anyway...");
                    }
                }
                else
                {
                    auto const& path = the_options()->get<std::string>(options::platform_path);
                    auto platforms = mir::libraries_for_path(path, *the_shared_library_prober_report());
                    if (platforms.empty())
                    {
                        auto msg = "Failed to find any platform plugins in: " + path;
                        throw std::runtime_error(msg.c_str());
                    }
                    platform_library = mir::graphics::module_for_device(platforms, dynamic_cast<mir::options::ProgramOption&>(*the_options()), the_console_services());
                }
                auto create_host_platform =
                    [platform_library]() -> std::function<std::remove_pointer<mg::CreateHostPlatform>::type>
                    {
                        try
                        {
                            return platform_library->load_function<mg::CreateHostPlatform>(
                                "create_host_platform",
                                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                        }
                        catch (std::runtime_error const&)
                        {
                            auto create_host_platform =
                                platform_library->load_function<mg::obsolete_0_27::CreateHostPlatform>(
                                    "create_host_platform",
                                    mg::obsolete_0_27::symbol_version);
                            return [create_host_platform](auto options, auto cleanup, auto, auto report, auto logger)
                                {
                                    return create_host_platform(options, cleanup, report, logger);
                                };
                        }
                    }();
                auto describe_module =
                    [platform_library]()
                    {
                        try
                        {
                            return platform_library->load_function<mg::DescribeModule>(
                                "describe_graphics_module",
                                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                        }
                        catch (std::runtime_error const&)
                        {
                            return platform_library->load_function<mg::DescribeModule>(
                                "describe_graphics_module",
                                mg::obsolete_0_27::symbol_version);
                        }
                    }();
                auto description = describe_module();
                mir::log_info("Selected driver: %s (version %d.%d.%d)",
                              description->name,
                              description->major_version,
                              description->minor_version,
                              description->micro_version);

                return create_host_platform(
                    the_options(),
                    the_emergency_cleanup(),
                    the_console_services(),
                    the_display_report(),
                    the_logger());
            }
            catch(...)
            {
                // access exception information before platform library gets unloaded
                error_report << "Exception while creating graphics platform" << std::endl;
                mir::report_exception(error_report);
            }
            BOOST_THROW_EXCEPTION(std::runtime_error(error_report.str()));
        });
}

std::shared_ptr<mg::GraphicBufferAllocator>
mir::DefaultServerConfiguration::the_buffer_allocator()
{
    return buffer_allocator(
        [&]()
        {
            return the_graphics_platform()->create_buffer_allocator(*the_display());
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
                if (auto egl_access = dynamic_cast<mir::renderer::gl::EGLPlatform*>(
                    the_graphics_platform()->native_rendering_platform()))
                {
                    return std::make_shared<mg::offscreen::Display>(
                        egl_access->egl_native_display(),
                        the_display_configuration_policy(),
                        the_display_report());
                }
                else
                {
                    BOOST_THROW_EXCEPTION(std::runtime_error(
                        "underlying rendering platform does not support EGL access."\
                        " Could not create offscreen display"));
                }
            }

            return the_graphics_platform()->create_display(
                the_display_configuration_policy(),
                the_gl_config());
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
