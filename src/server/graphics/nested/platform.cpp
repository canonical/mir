/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "platform.h"
#include "host_connection.h"
#include "display.h"
#include "buffer.h"
#include "native_buffer.h"
#include "ipc_operations.h"
#include "mir/shared_library.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/options/option.h"
#include "mir/options/configuration.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mo = mir::options;

namespace
{
mgn::PassthroughOption passthrough_from_options(mo::Option const& options)
{
    auto enabled = options.is_set(mo::nested_passthrough_opt) &&
        options.get<bool>(mo::nested_passthrough_opt);
    return enabled ? mgn::PassthroughOption::enabled : mgn::PassthroughOption::disabled;
}
}

mgn::Platform::Platform(
    std::shared_ptr<mir::SharedLibrary> const& library, 
    std::shared_ptr<mgn::HostConnection> const& connection, 
    std::shared_ptr<mg::DisplayReport> const& display_report,
    mo::Option const& options) :
    library(library),
    connection(connection),
    display_report(display_report),
    guest_platform(library->load_function<mg::CreateGuestPlatform>(
        "create_guest_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION)(display_report, connection)),
    passthrough_option(passthrough_from_options(options))
{
}

namespace
{
class BufferAllocator : public mg::GraphicBufferAllocator
{
public:
    BufferAllocator(
        std::shared_ptr<mgn::HostConnection> const& connection,
        std::shared_ptr<mg::GraphicBufferAllocator> const& guest_allocator) :
        connection(connection),
        guest_allocator(guest_allocator)
    {
    }

    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const& properties) override
    {
        if (passthrough_candidate(properties.size))
        {
            return std::make_shared<mgn::Buffer>(connection, properties.size, properties.format);
        }
        else
        {
            return guest_allocator->alloc_buffer(properties);
        }
    }

    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferRequestMessage const& properties)
    {
        if (passthrough_candidate(properties.size))
        {
            return std::make_shared<mgn::Buffer>(
                connection, properties.size, properties.native_format, properties.native_flags);
        }
        else
        {
            return guest_allocator->alloc_buffer(properties);
        }
    }

    std::vector<MirPixelFormat> supported_pixel_formats() override
    {
        return guest_allocator->supported_pixel_formats();
    }

private:
    bool passthrough_candidate(mir::geometry::Size size)
    {
        return (size.width >= mir::geometry::Width{480}) && (size.height >= mir::geometry::Height{480});
    }
    std::shared_ptr<mgn::HostConnection> const connection;
    std::shared_ptr<mg::GraphicBufferAllocator> const guest_allocator;
};
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgn::Platform::create_buffer_allocator()
{
    if (connection->supports_passthrough())
    {
        return mir::make_module_ptr<BufferAllocator>(connection, guest_platform->create_buffer_allocator());
    }
    else
    {
        return guest_platform->create_buffer_allocator();
    }
}

mir::UniqueModulePtr<mg::Display> mgn::Platform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& policy,
    std::shared_ptr<mg::GLConfig> const& config)
{
    return mir::make_module_ptr<mgn::Display>(
        guest_platform,
        connection,
        display_report,
        policy,
        config,
        passthrough_option);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgn::Platform::make_ipc_operations() const
{
    return mir::make_module_ptr<mgn::IpcOperations>(guest_platform->make_ipc_operations());
}
