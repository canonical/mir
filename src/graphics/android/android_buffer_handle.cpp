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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "android_buffer_handle_default.h"
#include "mir/compositor/buffer_ipc_package.h"

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

mga::AndroidBufferHandleDefault::AndroidBufferHandleDefault(ANativeWindowBuffer buf, geom::PixelFormat pf, BufferUsage use)
    : anw_buffer(buf),
      pixel_format(pf),
      buffer_usage(use)
{
    pack_ipc_package();
}

void mga::AndroidBufferHandleDefault::pack_ipc_package()
{
    ipc_package = std::make_shared<mc::BufferIPCPackage>();

    const native_handle_t *native_handle = anw_buffer.handle;

    /* pack int data */
    ipc_package->ipc_data.resize(native_handle->numInts);
    int fd_offset = native_handle->numFds;
    for(auto it=ipc_package->ipc_data.begin(); it != ipc_package->ipc_data.end(); it++)
    {
        *it = native_handle->data[fd_offset++];
    }

    /* pack fd data */
    ipc_package->ipc_fds.resize(native_handle->numFds);
    int offset = 0;
    for(auto it=ipc_package->ipc_fds.begin(); it != ipc_package->ipc_fds.end(); it++)
    {
        *it = native_handle->data[offset++];
    }

    /* pack platform independent data */
    ipc_package->stride = anw_buffer.stride;
}

EGLClientBuffer mga::AndroidBufferHandleDefault::get_egl_client_buffer() const
{
    return (EGLClientBuffer) &anw_buffer;
}

geom::Size mga::AndroidBufferHandleDefault::size() const
{
    return geom::Size{geom::Width{anw_buffer.width},
                      geom::Height{anw_buffer.height}};
}

geom::Stride mga::AndroidBufferHandleDefault::stride() const
{
    return geom::Stride(anw_buffer.stride);
}

geom::PixelFormat mga::AndroidBufferHandleDefault::format() const
{
    return pixel_format;
}

mga::BufferUsage mga::AndroidBufferHandleDefault::usage() const
{
    return buffer_usage;
}

std::shared_ptr<mc::BufferIPCPackage> mga::AndroidBufferHandleDefault::get_ipc_package() const
{
    return ipc_package;
}
