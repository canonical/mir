/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/android_buffer_ipc_package.h"

#include <system/window.h>

namespace mga=mir::graphics::android;

mga::AndroidBufferIPCPackage::AndroidBufferIPCPackage(const AndroidBufferHandle* handle)
{
    ANativeWindowBuffer *buffer = (ANativeWindowBuffer*) handle->get_egl_client_buffer();
    const native_handle_t *native_handle = buffer->handle;

    /* todo: check version */

    /* pack int data */
    ipc_data.resize(native_handle->numInts); 
    int fd_offset = native_handle->numFds;
    for(auto it=ipc_data.begin(); it != ipc_data.end(); it++)
    {
        *it = native_handle->data[fd_offset++];
    }

    /* pack fd data */ 
}

std::vector<int> mga::AndroidBufferIPCPackage::get_ipc_data()
{
    return ipc_data;
}
