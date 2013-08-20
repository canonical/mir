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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "mir/graphics/nested/nested_platform.h"
#include "mir/graphics/nested/nested_mir_connection_handle.h"
#include "mir_toolkit/mir_client_library.h"

#include "nested_display.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mo = mir::options;

mgn::NestedPlatform::NestedPlatform(
    std::string const& host,
    std::shared_ptr<mg::DisplayReport> const& display_report,
    std::shared_ptr<mg::NativePlatform> const& native_platform) :
native_platform{native_platform},
display_report{display_report},
connection{mir_connect_sync(host.c_str(), "nested_mir")}
{
    if (!mir_connection_is_valid(connection))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Platform Connection Error: " + std::string(mir_connection_get_error_message(connection))));
    }
}

mgn::NestedPlatform::~NestedPlatform() noexcept
{
}

std::shared_ptr<mg::GraphicBufferAllocator> mgn::NestedPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return native_platform->create_buffer_allocator(buffer_initializer);
}

std::shared_ptr<mg::Display> mgn::NestedPlatform::create_display(std::shared_ptr<mg::DisplayConfigurationPolicy> const& /*initial_conf_policy*/)
{
    return std::make_shared<mgn::NestedDisplay>(connection, display_report);
}

std::shared_ptr<mg::PlatformIPCPackage> mgn::NestedPlatform::get_ipc_package()
{
    return native_platform->get_ipc_package();
}

std::shared_ptr<mg::InternalClient> mgn::NestedPlatform::create_internal_client()
{
    return native_platform->create_internal_client();
}

void mgn::NestedPlatform::fill_ipc_package(std::shared_ptr<BufferIPCPacker> const& packer,
                                        std::shared_ptr<Buffer> const& buffer) const
{
    native_platform->fill_ipc_package(packer, buffer);
}
