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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/buffer.h"
#include "server_render_window.h"
#include "display_support_provider.h"
#include "fb_swapper.h"
#include "buffer.h"
#include "android_format_conversion-inl.h"
#include "interpreter_resource_cache.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <thread>
#include <chrono>
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::ServerRenderWindow::ServerRenderWindow(std::shared_ptr<mga::FBSwapper> const& swapper,
                                            std::shared_ptr<mga::DisplaySupportProvider> const& display_poster,
                                            std::shared_ptr<InterpreterResourceCache> const& cache)
    : swapper(swapper),
      poster(display_poster),
      resource_cache(cache),
      format(mga::to_android_format(poster->display_format()))
{
}

ANativeWindowBuffer* mga::ServerRenderWindow::driver_requests_buffer()
{
    auto buffer = swapper->compositor_acquire();
    auto handle = buffer->native_buffer_handle().get();
    resource_cache->store_buffer(buffer, handle);
    return handle;
}

//sync object could be passed to hwc. we don't need to that yet though
void mga::ServerRenderWindow::driver_returns_buffer(ANativeWindowBuffer* returned_handle, std::shared_ptr<SyncObject> const& fence)
{
    fence->wait();
    auto buffer = resource_cache->retrieve_buffer(returned_handle); 
    poster->set_next_frontbuffer(buffer);
    swapper->compositor_release(buffer);
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
            size = poster->display_size();
            return size.width.as_uint32_t();
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
        case NATIVE_WINDOW_HEIGHT:
            size = poster->display_size();
            return size.height.as_uint32_t();
        case NATIVE_WINDOW_FORMAT:
            return format;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            return 0; 
        default:
            BOOST_THROW_EXCEPTION(std::runtime_error("driver requests info we dont provide. key: " + key));
    }
}

void mga::ServerRenderWindow::sync_to_display(bool sync)
{
    poster->sync_to_display(sync);
}
