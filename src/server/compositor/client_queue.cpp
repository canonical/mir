/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "client_queue.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

void mc::ClientQueue::schedule(std::shared_ptr<graphics::Buffer> const& buffer)
{
    printf("POPIN\n");
    std::unique_lock<decltype(mutex)> lk(mutex);
    auto it = std::find(queue.begin(), queue.end(), buffer);
    if (it != queue.end())
    {
        printf("A REPLACE?\n");
        queue.erase(it);
    }
    queue.emplace_back(buffer);
}

void mc::ClientQueue::cancel(std::shared_ptr<graphics::Buffer> const& buffer)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    auto it = std::find(queue.begin(), queue.end(), buffer);
    if (it != queue.end())
        queue.erase(it);
}

bool mc::ClientQueue::anything_scheduled()
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    printf("QUEUE SIZE %i\n", (int) queue.size());
    return !queue.empty();
}

std::shared_ptr<mg::Buffer> mc::ClientQueue::next_buffer()
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    if (queue.empty())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer scheduled"));
    auto buffer = queue.front();
    queue.pop_front();
    return buffer;
}
