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

#include "nested_platform.h"
#include "host_connection.h"
#include "mir/graphics/nested_context.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_drm.h"

#include "nested_display.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mo = mir::options;

namespace
{

class MirConnectionNestedContext : public mg::NestedContext
{
public:
    MirConnectionNestedContext(std::shared_ptr<mgn::HostConnection> const& connection)
        : connection{connection}
    {
    }

    std::vector<int> platform_fd_items()
    {
        MirPlatformPackage pkg;
        mir_connection_get_platform(*connection, &pkg);
        return std::vector<int>(pkg.fd, pkg.fd + pkg.fd_items);
    }

    void drm_auth_magic(int magic)
    {
        int status;
        mir_wait_for(mir_connection_drm_auth_magic(*connection, magic,
                                                   drm_auth_magic_callback, &status));
        if (status)
        {
            std::string const msg("Nested Mir failed to authenticate magic");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(status));
        }
    }

    void drm_set_gbm_device(struct gbm_device* dev)
    {
        if (!mir_connection_drm_set_gbm_device(*connection, dev))
        {
            std::string const msg("Nested Mir failed to set the gbm device");
            BOOST_THROW_EXCEPTION(std::runtime_error(msg));
        }
    }

    static void drm_auth_magic_callback(int status, void* context)
    {
        int* status_ret = static_cast<int*>(context);
        *status_ret = status;
    }

private:
    std::shared_ptr<mgn::HostConnection> const connection;
};

}

mgn::NestedPlatform::NestedPlatform(
    std::shared_ptr<HostConnection> const& connection,
    std::shared_ptr<input::EventFilter> const& event_handler,
    std::shared_ptr<mg::DisplayReport> const& display_report,
    std::shared_ptr<mg::NativePlatform> const& native_platform) :
native_platform{native_platform},
event_handler{event_handler},
display_report{display_report},
connection{connection}
{
    if (!mir_connection_is_valid(*connection))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Platform Connection Error: " + std::string(mir_connection_get_error_message(*connection))));
    }

    native_platform->initialize(std::make_shared<MirConnectionNestedContext>(connection));
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
    return std::make_shared<mgn::NestedDisplay>(connection, event_handler, display_report);
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
