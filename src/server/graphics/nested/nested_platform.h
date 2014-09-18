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
#include "host_connection.h"

namespace mir
{
namespace input { class InputDispatcher; }
namespace graphics
{
namespace nested
{

class NestedPlatform : public Platform
{
public:
    NestedPlatform(
        std::shared_ptr<HostConnection> const& connection,
        std::shared_ptr<input::InputDispatcher> const& dispatcher,
        std::shared_ptr<DisplayReport> const& display_report,
        std::shared_ptr<NativePlatform> const& native_platform);

    ~NestedPlatform() noexcept;
    std::shared_ptr<GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<BufferInitializer> const& buffer_initializer) override;
    std::shared_ptr<Display> create_display(
            std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
            std::shared_ptr<GLProgramFactory> const& gl_program_factory,
            std::shared_ptr<GLConfig> const& gl_config);
    std::shared_ptr<PlatformIPCPackage> get_ipc_package() override;
    std::shared_ptr<InternalClient> create_internal_client() override;
    std::shared_ptr<BufferWriter> make_buffer_writer() override;
    void fill_buffer_package(
        BufferIPCPacker* packer, Buffer const* Buffer, BufferIpcMsgType msg_type) const override;
    EGLNativeDisplayType egl_native_display() const;

private:
    std::shared_ptr<NativePlatform> const native_platform;
    std::shared_ptr<input::InputDispatcher> const dispatcher;
    std::shared_ptr<DisplayReport> const display_report;
    std::shared_ptr<HostConnection> const connection;
};

}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_PLATFORM_H
