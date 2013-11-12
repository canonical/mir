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

#ifndef MIR_GRAPHICS_NESTED_NESTED_PLATFORM_H_
#define MIR_GRAPHICS_NESTED_NESTED_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/graphics/native_platform.h"
#include "mir/graphics/nested/host_connection.h"

namespace mir
{
namespace input { class EventFilter; }
namespace graphics
{
namespace nested
{

class NestedPlatform : public Platform
{
public:
    NestedPlatform(
        std::shared_ptr<HostConnection> const& connection,
        std::shared_ptr<input::EventFilter> const& event_handler,
        std::shared_ptr<DisplayReport> const& display_report,
        std::shared_ptr<NativePlatform> const& native_platform);

    ~NestedPlatform() noexcept;
    std::shared_ptr<GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<BufferInitializer> const& buffer_initializer);
    std::shared_ptr<Display> create_display(
            std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy);
    std::shared_ptr<PlatformIPCPackage> get_ipc_package();
    std::shared_ptr<InternalClient> create_internal_client();
    void fill_ipc_package(std::shared_ptr<BufferIPCPacker> const& packer,
                          std::shared_ptr<Buffer> const& Buffer) const;

private:
    std::shared_ptr<NativePlatform> const native_platform;
    std::shared_ptr<input::EventFilter> const event_handler;
    std::shared_ptr<DisplayReport> const display_report;
    std::shared_ptr<HostConnection> const connection;
};

}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_PLATFORM_H
