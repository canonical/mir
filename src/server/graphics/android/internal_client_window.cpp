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
#include "internal_client_window.h"

namespace mga=mir::graphics::android;

mga::InternalClientWindow::InternalClientWindow(std::unique_ptr<compositor::BufferSwapper>&& swapper)
    : swapper(std::move(swapper))
{
}

ANativeWindowBuffer* mga::InternalClientWindow::driver_requests_buffer()
{
    return nullptr;
}

void mga::InternalClientWindow::driver_returns_buffer(ANativeWindowBuffer*, std::shared_ptr<SyncObject> const&)
{
}

void mga::InternalClientWindow::dispatch_driver_request_format(int)
{
}

int mga::InternalClientWindow::driver_requests_info(int) const
{
    return 8;
}
