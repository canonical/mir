/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "internal_client_window.h"
#include "mir/graphics/internal_surface.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/buffer.h"
#include "interpreter_resource_cache.h"
#include "android_format_conversion-inl.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>

namespace mg=mir::graphics;
namespace mga=mg::android;
namespace geom=mir::geometry;

mga::InternalClientWindow::InternalClientWindow(std::shared_ptr<InternalSurface> const& surface)
    : surface(surface),
      buffer(nullptr)
{
    format = mga::to_android_format(MirPixelFormat(surface->pixel_format()));
}

mg::NativeBuffer* mga::InternalClientWindow::driver_requests_buffer()
{
    if (!buffer)
    {
        surface->swap_buffers(buffer);
    }

    auto handle = buffer->native_buffer_handle();
    lookup[handle->anwb()] = {buffer, handle};

    buffer = nullptr;

    return handle.get();
}

void mga::InternalClientWindow::driver_returns_buffer(ANativeWindowBuffer* key, int fence_fd)
{
    auto it = lookup.find(key);
    if (it == lookup.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("driver is returning buffers it never was given!"));
    }

    auto handle = it->second.handle;
    buffer = it->second.buffer;
    lookup.erase(it);

    handle->update_fence(fence_fd);
    surface->swap_buffers(buffer);
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
            {
            std::stringstream sstream;
            sstream << "driver requests info we dont provide. key: " << key;
            BOOST_THROW_EXCEPTION(std::runtime_error(sstream.str()));
            }
    }
}

void mga::InternalClientWindow::sync_to_display(bool)
{
    //note: clients run with the swapinterval of the display. ignore their request for now
}
