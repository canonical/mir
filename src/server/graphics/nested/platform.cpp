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
#include "mir/shared_library.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;

mgn::Platform::Platform(
    std::shared_ptr<mir::SharedLibrary> const& library, 
    std::shared_ptr<mgn::HostConnection> const& connection, 
    std::shared_ptr<mg::DisplayReport> const& display_report):
    library(library),
    connection(connection),
    display_report(display_report),
    guest_platform(library->load_function<mg::CreateGuestPlatform>(
        "create_guest_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION)(display_report, connection))
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgn::Platform::create_buffer_allocator()
{
    return guest_platform->create_buffer_allocator();
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
        config);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgn::Platform::make_ipc_operations() const
{
    return guest_platform->make_ipc_operations();
}
