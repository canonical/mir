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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_CLIENT_CONTEXT_H_
#define MIR_CLIENT_CLIENT_CONTEXT_H_

#include "mir_toolkit/client_types.h"
#include "mir/mir_render_surface.h"
#include "mir/geometry/size.h"

namespace mir
{
namespace client
{
class MirBuffer;
class ClientContext
{
public:
    virtual ~ClientContext() {}

    virtual void populate_server_package(MirPlatformPackage& platform_package) = 0;
    virtual void populate_graphics_module(MirModuleProperties& graphics_module) = 0;
    virtual MirWaitHandle* platform_operation(
        MirPlatformMessage const* request,
        MirPlatformOperationCallback callback, void* context) = 0;
    virtual void allocate_buffer(
        mir::geometry::Size size, MirPixelFormat format,
        MirBufferCallback callback, void* context) = 0;
    virtual void allocate_buffer(
        mir::geometry::Size size, uint32_t native_format, uint32_t native_flags,
        MirBufferCallback callback, void* context) = 0;
    virtual void release_buffer(mir::client::MirBuffer* buffer) = 0;

protected:
    ClientContext() = default;
    ClientContext(const ClientContext&) = delete;
    ClientContext& operator=(const ClientContext&) = delete;
};

ClientContext* to_client_context(MirConnection*);

}
}

#endif /* MIR_CLIENT_CLIENT_CONTEXT_H_ */
