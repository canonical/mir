/*
 * Copyright © 2013 Canonical Ltd.
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

#include "client_surface_interpreter.h"
#include "mir/graphics/android/sync_fence.h"
#include "../client_buffer.h"
#include <system/window.h>
#include <stdexcept>

namespace mcla=mir::client::android;
namespace mga=mir::graphics::android;

mcla::ClientSurfaceInterpreter::ClientSurfaceInterpreter(ClientSurface& surface)
 :  surface(surface),
    driver_pixel_format(-1),
    sync_ops(std::make_shared<mga::RealSyncFileOps>())

{
}

mir::graphics::NativeBuffer* mcla::ClientSurfaceInterpreter::driver_requests_buffer()
{
    auto buffer = surface.get_current_buffer();
    auto buffer_to_driver = buffer->native_buffer_handle();

    ANativeWindowBuffer* anwb = buffer_to_driver->anwb();
    anwb->format = driver_pixel_format;
    return buffer_to_driver.get();
}

void mcla::ClientSurfaceInterpreter::driver_returns_buffer(ANativeWindowBuffer*, int fence_fd)
{
    //TODO: pass fence to server instead of waiting here
    mga::SyncFence sync_fence(sync_ops, fence_fd);
    sync_fence.wait();

    surface.request_and_wait_for_next_buffer();
}

void mcla::ClientSurfaceInterpreter::dispatch_driver_request_format(int format)
{
    driver_pixel_format = format;
}

int mcla::ClientSurfaceInterpreter::driver_requests_info(int key) const
{
    switch (key)
    {
        case NATIVE_WINDOW_WIDTH:
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            return surface.get_parameters().width;
        case NATIVE_WINDOW_HEIGHT:
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            return surface.get_parameters().height;
        case NATIVE_WINDOW_FORMAT:
            return driver_pixel_format;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            return 0;
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            return 2;
        default:
            throw std::runtime_error("driver requested unsupported query");
    }
}

void mcla::ClientSurfaceInterpreter::sync_to_display(bool)
{
    //note: clients run with the swapinterval of the display. ignore their request for now
}
