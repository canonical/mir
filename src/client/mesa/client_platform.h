/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MESA_CLIENT_PLATFORM_H_
#define MIR_CLIENT_MESA_CLIENT_PLATFORM_H_

#include "../client_platform.h"

namespace mir
{
namespace client
{
class ClientBufferDepository;
class EGLNativeDisplayContainer;

namespace mesa
{

class BufferFileOps;

class ClientPlatform : public client::ClientPlatform
{
public:
    ClientPlatform(ClientContext* const context,
                   std::shared_ptr<BufferFileOps> const& buffer_file_ops,
                   EGLNativeDisplayContainer& display_container);

    MirPlatformType platform_type() const;
    std::shared_ptr<ClientBufferFactory> create_buffer_factory();
    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(ClientSurface *surface);
    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display();
    MirNativeBuffer* convert_native_buffer(graphics::NativeBuffer*) const;

private:
    ClientContext* const context;
    std::shared_ptr<BufferFileOps> const buffer_file_ops;
    EGLNativeDisplayContainer& display_container;
};

}
}
}

#endif /* MIR_CLIENT_MESA_CLIENT_PLATFORM_H_ */
