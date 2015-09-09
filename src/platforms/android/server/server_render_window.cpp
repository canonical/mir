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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/android_format_conversion-inl.h"
#include "server_render_window.h"
#include "framebuffer_bundle.h"
#include "buffer.h"
#include "interpreter_resource_cache.h"

#include <system/window.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::ServerRenderWindow::ServerRenderWindow(
    std::shared_ptr<mga::FramebufferBundle> const& fb_bundle,
    MirPixelFormat format,
    std::shared_ptr<InterpreterResourceCache> const& cache)
    : fb_bundle(fb_bundle),
      resource_cache(cache),
      format(mga::to_android_format(format))
{
}

mg::NativeBuffer* mga::ServerRenderWindow::driver_requests_buffer()
{
    auto buffer = fb_bundle->buffer_for_render();
    auto handle = buffer->native_buffer_handle();
    resource_cache->store_buffer(buffer, handle);
    return handle.get();
}

void mga::ServerRenderWindow::driver_returns_buffer(ANativeWindowBuffer* buffer, int fence_fd)
{
    resource_cache->update_native_fence(buffer, fence_fd);
    resource_cache->retrieve_buffer(buffer);
}

void mga::ServerRenderWindow::dispatch_driver_request_format(int request_format)
{
    format = request_format;
}

int mga::ServerRenderWindow::driver_requests_info(int key) const
{
    geom::Size size;
    switch(key)
    {
        case NATIVE_WINDOW_DEFAULT_WIDTH:
        case NATIVE_WINDOW_WIDTH:
            size = fb_bundle->fb_size();
            return size.width.as_uint32_t();
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
        case NATIVE_WINDOW_HEIGHT:
            size = fb_bundle->fb_size();
            return size.height.as_uint32_t();
        case NATIVE_WINDOW_FORMAT:
            return format;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            return 0;
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            return 1;
        case NATIVE_WINDOW_CONCRETE_TYPE:
            return NATIVE_WINDOW_FRAMEBUFFER;
        case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
            return GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_FB;
        default:
            {
            std::stringstream sstream;
            sstream << "driver requests info we dont provide. key: " << key;
            BOOST_THROW_EXCEPTION(std::runtime_error(sstream.str()));
            }
    }
}

void mga::ServerRenderWindow::sync_to_display(bool)
{
}

void mga::ServerRenderWindow::dispatch_driver_request_buffer_count(unsigned int)
{
    //note: Haven't seen a good reason to honor this request for a fb context
}
