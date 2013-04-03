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
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mc=mir::compositor;
namespace mga=mir::graphics::android;

mga::ServerRenderWindow::ServerRenderWindow(std::shared_ptr<compositor::BufferSwapper> const& swapper)
    : swapper(swapper)
{
}

ANativeWindowBuffer* mga::ServerRenderWindow::driver_requests_buffer()
{
    auto buffer = swapper->compositor_acquire();
    auto handle = buffer->native_buffer_handle().get();
    buffers_in_driver[(ANativeWindowBuffer*) handle] = buffer;
    return handle;
}

void mga::ServerRenderWindow::driver_returns_buffer(ANativeWindowBuffer* returned_handle)
{
    auto buffer_it = buffers_in_driver.find(returned_handle); 
    if (buffer_it == buffers_in_driver.end())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("driver is returning buffers it never was given!"));
    }

    auto buffer = buffer_it->second;
    buffers_in_driver.erase(buffer_it);
    swapper->compositor_release(buffer);
}

void mga::ServerRenderWindow::dispatch_driver_request_format(int /*format*/)
{
}

int mga::ServerRenderWindow::driver_requests_info(int /*key*/) const
{
    return 8;
}
