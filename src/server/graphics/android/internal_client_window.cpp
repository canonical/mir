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

#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer.h"
#include "internal_client_window.h"
#include "interpreter_resource_cache.h"

namespace mga=mir::graphics::android;

mga::InternalClientWindow::InternalClientWindow(std::unique_ptr<compositor::BufferSwapper>&& swapper,
                                                std::shared_ptr<InterpreterResourceCache> const& cache)
    : swapper(std::move(swapper)),
      resource_cache(cache)
{
}

ANativeWindowBuffer* mga::InternalClientWindow::driver_requests_buffer()
{
    auto buffer = swapper->client_acquire();
    auto handle = buffer->native_buffer_handle().get();
    resource_cache->store_buffer(buffer, handle);
    return handle;
}

void mga::InternalClientWindow::driver_returns_buffer(ANativeWindowBuffer* handle, std::shared_ptr<SyncObject> const&)
{
    auto buffer = resource_cache->retrieve_buffer(handle);
    swapper->client_release(buffer);
}

void mga::InternalClientWindow::dispatch_driver_request_format(int)
{
}

int mga::InternalClientWindow::driver_requests_info(int) const
{
    return 8;
}
