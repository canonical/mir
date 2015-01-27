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
#include "nested/mir_client_host_connection.h"
#include "nested/nested_display.h"
#include "mir/graphics/nested_context.h"
#include "offscreen/display.h"
#include "software_cursor.h"

#include "mir/graphics/gl_config.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/platform_probe.h"
#include "program_factory.h"

#include "mir/shared_library.h"
#include "mir/shared_library_prober.h"
#include "mir/abnormal_exit.h"
#include "mir/emergency_cleanup.h"
#include "mir/log.h"

#include "mir_toolkit/common.h"

#include <boost/throw_exception.hpp>

#include <map>

namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace mgn = mir::graphics::nested;

namespace
{
// TODO: Temporary, until we actually manage module lifetimes
static std::shared_ptr<mir::SharedLibrary> platform_library;
}

std::shared_ptr<mg::DisplayConfigurationPolicy>
mir::DefaultServerConfiguration::the_display_configuration_policy()
{
    return display_configuration_policy(
        [this]
        {
            return wrap_display_configuration_policy(
                std::make_shared<mg::DefaultDisplayConfigurationPolicy>());
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
//TODO: what is the point of NestedContext if its just the same as mgn:HostConnection?
class MirConnectionNestedContext : public mg::NestedContext
{
public:
    MirConnectionNestedContext(std::shared_ptr<mgn::HostConnection> const& connection)
        : connection{connection}
    {
    }

    std::vector<int> platform_fd_items()
    {
        return connection->platform_fd_items();
    }

    void drm_auth_magic(int magic)
    {
        connection->drm_auth_magic(magic);
    }

    void drm_set_gbm_device(struct gbm_device* dev)
    {
        connection->drm_set_gbm_device(dev);
    }

private:
    std::shared_ptr<mgn::HostConnection> const connection;
};
}

std::shared_ptr<mg::Platform> mir::DefaultServerConfiguration::the_graphics_platform()
{
    return graphics_platform(
        [this]()->std::shared_ptr<mg::Platform>
        {
            // fallback to standalone if host socket is unset
            if (the_options()->is_set(options::platform_graphics_lib))
            {
                platform_library = std::make_shared<mir::SharedLibrary>(the_options()->get<std::string>(options::platform_graphics_lib));
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
                platform_library = mir::graphics::module_for_device(platforms);
            }
            auto create_host_platform = platform_library->load_function<mg::CreateHostPlatform>(
                "create_host_platform",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            auto create_guest_platform = platform_library->load_function<mg::CreateGuestPlatform>(
                "create_guest_platform",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            auto describe_module = platform_library->load_function<mg::DescribeModule>(
                "describe_graphics_module",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            auto description = describe_module();
            ml::log(ml::Severity::informational,
                    std::string{"Selected driver: "} + description->name + " (version " +
                    std::to_string(description->major_version) + "." +
                    std::to_string(description->minor_version) + "." +
                    std::to_string(description->micro_version) + ")",
                    "Platform Loader");

            if (!the_options()->is_set(options::host_socket_opt))
                return create_host_platform(the_options(), the_emergency_cleanup(), the_display_report());
            else
                return create_guest_platform(
                    the_display_report(),
                    std::make_shared<MirConnectionNestedContext>(the_host_connection()));

        });
}

std::shared_ptr<mg::GraphicBufferAllocator>
mir::DefaultServerConfiguration::the_buffer_allocator()
{
    return buffer_allocator(
        [&]()
        {
            return the_graphics_platform()->create_buffer_allocator();
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
                    the_graphics_platform()->egl_native_display(),
                    the_display_configuration_policy(),
                    the_display_report());
            }
            else if (the_options()->is_set(options::host_socket_opt))
            {
                return std::make_shared<mgn::NestedDisplay>(
                    the_graphics_platform(),
                    the_host_connection(),
                    the_input_dispatcher(),
                    the_display_report(),
                    the_display_configuration_policy(),
                    the_gl_config());
            }
            {
                return the_graphics_platform()->create_display(
                    the_display_configuration_policy(),
                    the_gl_program_factory(),
                    the_gl_config());
            }
        });
}

std::shared_ptr<mg::Cursor>
mir::DefaultServerConfiguration::the_cursor()
{
    return cursor(
        [this]() -> std::shared_ptr<mg::Cursor>
        {
            // We try to create a hardware cursor, if this fails we use a software cursor
            auto hardware_cursor = the_display()->create_hardware_cursor(the_default_cursor_image());
            if (hardware_cursor)
            {
                mir::log_info("Using hardware cursor");
                return hardware_cursor;
            }
            else
            {
                mir::log_info("Using software cursor");

                auto const cursor = std::make_shared<mg::SoftwareCursor>(
                    the_buffer_allocator(),
                    the_input_scene());

                cursor->show(*the_default_cursor_image());

                return cursor;
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

            if (!options->is_set(options::host_socket_opt))
                BOOST_THROW_EXCEPTION(mir::AbnormalExit(
                    std::string("Exiting Mir! Reason: Nested Mir needs either $MIR_SOCKET or --") +
                    options::host_socket_opt));

            auto host_socket = options->get<std::string>(options::host_socket_opt);

            std::string server_socket{"none"};

            if (!the_options()->is_set(options::no_server_socket_opt))
            {
                server_socket = the_socket_file();

                if (server_socket == host_socket)
                    BOOST_THROW_EXCEPTION(mir::AbnormalExit(
                        "Exiting Mir! Reason: Nested Mir and Host Mir cannot use "
                        "the same socket file to accept connections!"));
            }

            auto const my_name = options->is_set(options::name_opt) ?
                options->get<std::string>(options::name_opt) :
                "nested-mir@:" + server_socket;

            return std::make_shared<graphics::nested::MirClientHostConnection>(
                host_socket,
                my_name,
                the_host_lifecycle_event_listener());
        });
}

std::shared_ptr<mg::GLConfig>
mir::DefaultServerConfiguration::the_gl_config()
{
    return gl_config(
        [this]
        {
            struct NoGLConfig : public mg::GLConfig
            {
                int depth_buffer_bits() const override { return 0; }
                int stencil_buffer_bits() const override { return 0; }
            };
            return std::make_shared<NoGLConfig>();
        });
}

std::shared_ptr<mg::GLProgramFactory>
mir::DefaultServerConfiguration::the_gl_program_factory()
{
    return gl_program_factory(
        [this]
        {
            return std::make_shared<mg::ProgramFactory>();
        });
}
