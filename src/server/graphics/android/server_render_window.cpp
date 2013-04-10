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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "server_render_window.h"
#include "native_buffer_handle.h"
#include "display_info_provider.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::ServerRenderWindow::ServerRenderWindow(std::shared_ptr<mc::BufferSwapper> const& swapper,
                                            std::shared_ptr<mga::DisplayInfoProvider> const& display_poster)
    : swapper(swapper),
      poster(display_poster),
      native_format(0)
{
}

ANativeWindowBuffer* mga::ServerRenderWindow::driver_requests_buffer()
{
    printf("acquire buffer\n");
    auto buffer = swapper->compositor_acquire();
    auto handle = buffer->native_buffer_handle().get();
    buffers_in_driver[(ANativeWindowBuffer*) handle] = buffer;

    printf("return %X\n", (int)handle);
    return handle;
}

void mga::ServerRenderWindow::driver_returns_buffer(ANativeWindowBuffer* returned_handle)
{
    printf("return buffer\n");
    auto buffer_it = buffers_in_driver.find(returned_handle); 
    if (buffer_it == buffers_in_driver.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("driver is returning buffers it never was given!"));
    }

    auto buffer = buffer_it->second;
    buffers_in_driver.erase(buffer_it);
    poster->set_next_frontbuffer(buffer);
    swapper->compositor_release(buffer);
}

void mga::ServerRenderWindow::dispatch_driver_request_format(int /*format*/)
{
}

int mga::ServerRenderWindow::driver_requests_info(int key) const
{
    geom::Size size;
    switch(key)
    {
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            size = poster->display_size();
            return size.width.as_uint32_t();
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            size = poster->display_size();
            return size.height.as_uint32_t();
        case NATIVE_WINDOW_FORMAT:
            return HAL_PIXEL_FORMAT_RGBX_8888 ;
        default:
            BOOST_THROW_EXCEPTION(std::runtime_error("driver requests info we dont provide. key: " + key));
            return -1;
    }
}
