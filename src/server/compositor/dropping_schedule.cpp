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

#include "dropping_schedule.h"
#include "mir/frontend/client_buffers.h"
#include "mir/graphics/buffer.h"

#include <boost/throw_exception.hpp>
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

mc::DroppingSchedule::DroppingSchedule(std::shared_ptr<mf::ClientBuffers> const& client_buffers) :
    sender(client_buffers)
{
}

void mc::DroppingSchedule::schedule(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if ((the_only_buffer != buffer) && the_only_buffer)
        sender->send_buffer(the_only_buffer->id());
    the_only_buffer = buffer;
}

bool mc::DroppingSchedule::anything_scheduled()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    return static_cast<bool>(the_only_buffer);
}

std::shared_ptr<mg::Buffer> mc::DroppingSchedule::next_buffer()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (!the_only_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer scheduled"));
    auto buffer = the_only_buffer;
    the_only_buffer = nullptr;
    return buffer;
}
