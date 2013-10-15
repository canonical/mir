/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "internal_client_window.h"
#include "mir/graphics/internal_surface.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/buffer.h"
#include "interpreter_resource_cache.h"
#include "android_format_conversion-inl.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mg::android;
namespace geom=mir::geometry;

mga::InternalClientWindow::InternalClientWindow(std::shared_ptr<InternalSurface> const& surface,
                                                std::shared_ptr<InterpreterResourceCache> const& cache)
    : surface(surface),
      resource_cache(cache)
{
    format = mga::to_android_format(geometry::PixelFormat(surface->pixel_format()));
}

mg::NativeBuffer* mga::InternalClientWindow::driver_requests_buffer()
{
    auto buffer = surface->advance_client_buffer();
    auto handle = buffer->native_buffer_handle();
    resource_cache->store_buffer(buffer, handle);
    return handle.get();
}

void mga::InternalClientWindow::driver_returns_buffer(ANativeWindowBuffer* buffer, int fence_fd)
{
    resource_cache->update_native_fence(buffer, fence_fd);
    resource_cache->retrieve_buffer(buffer);
    /* here, the mc::TemporaryBuffer will destruct, triggering buffer advance */
}

void mga::InternalClientWindow::dispatch_driver_request_format(int request_format)
{
    format = request_format;
}

int mga::InternalClientWindow::driver_requests_info(int key) const
{
    geom::Size size;
    switch(key)
    {
        case NATIVE_WINDOW_DEFAULT_WIDTH:
        case NATIVE_WINDOW_WIDTH:
            size = surface->size();
            return size.width.as_uint32_t();
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
        case NATIVE_WINDOW_HEIGHT:
            size = surface->size();
            return size.height.as_uint32_t();
        case NATIVE_WINDOW_FORMAT:
            return format;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            return 0; 
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            return 1;
        default:
            BOOST_THROW_EXCEPTION(std::runtime_error("driver requests info we dont provide. key: " + key));
    }
}

void mga::InternalClientWindow::sync_to_display(bool)
{
    //note: clients run with the swapinterval of the display. ignore their request for now
}
