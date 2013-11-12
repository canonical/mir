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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "fb_simple_swapper.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;

std::shared_ptr<mg::Buffer> mga::FBSimpleSwapper::compositor_acquire()
{
    std::unique_lock<std::mutex> lk(queue_lock);
    while (queue.empty())
    {
        cv.wait(lk);
    }

    auto buffer = queue.front();
    queue.pop();
    return buffer;
}

void mga::FBSimpleSwapper::compositor_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lk(queue_lock);

    queue.push(released_buffer);
    cv.notify_all();
}
