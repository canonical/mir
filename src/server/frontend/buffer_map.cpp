/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/graphics/buffer.h"
#include "mir/frontend/buffer_sink.h"
#include "buffer_map.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

namespace mir
{
namespace frontend
{
enum class BufferMap::Owner
{
    server,
    client
};
}
}

mf::BufferMap::BufferMap() = default;

mg::BufferID mf::BufferMap::add_buffer(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    buffers[buffer->id()] = {buffer, Owner::client};
    return buffer->id();
}

void mf::BufferMap::remove_buffer(mg::BufferID id)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    auto it = checked_buffers_find(id, lk);
    buffers.erase(it);
}

std::shared_ptr<mg::Buffer> mf::BufferMap::get(mg::BufferID id) const
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    return checked_buffers_find(id, lk)->second.buffer;
}

mf::BufferMap::Map::iterator mf::BufferMap::checked_buffers_find(
    mg::BufferID id, std::unique_lock<std::mutex> const&)
{
    auto it = buffers.find(id);
    if (it == buffers.end())
        BOOST_THROW_EXCEPTION(std::logic_error("cannot find buffer by id"));
    return it;
}

mf::BufferMap::Map::const_iterator mf::BufferMap::checked_buffers_find(
    mg::BufferID id, std::unique_lock<std::mutex> const&) const
{
    auto it = buffers.find(id);
    if (it == buffers.end())
        BOOST_THROW_EXCEPTION(std::logic_error("cannot find buffer by id"));
    return it;
}
