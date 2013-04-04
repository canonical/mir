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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "fb_swapper.h"

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

template<class T>
void mga::FBSwapper::initialize_queues(T /* buffer_list */)
{
}

mga::FBSwapper::FBSwapper(std::initializer_list<std::shared_ptr<mc::Buffer>> /* buffer_list*/)
{
}
mga::FBSwapper::FBSwapper(std::vector<std::shared_ptr<mc::Buffer>> /*buffer_list*/)
{
}

std::shared_ptr<mc::Buffer> mga::FBSwapper::client_acquire()
{
    return std::shared_ptr<mc::Buffer>(); 
}

void mga::FBSwapper::client_release(std::shared_ptr<mc::Buffer> const& /*queued_buffer*/)
{
}

std::shared_ptr<mc::Buffer> mga::FBSwapper::compositor_acquire()
{
    return std::shared_ptr<mc::Buffer>(); 
}

void mga::FBSwapper::compositor_release(std::shared_ptr<mc::Buffer> const& /*released_buffer*/)
{
}

void mga::FBSwapper::shutdown()
{
}

