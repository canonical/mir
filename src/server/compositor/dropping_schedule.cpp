/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "mir/graphics/buffer.h"

#include <boost/throw_exception.hpp>
namespace mg = mir::graphics;
namespace mc = mir::compositor;

mc::DroppingSchedule::DroppingSchedule()
{
}

void mc::DroppingSchedule::schedule(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    the_only_buffer = buffer;
}

unsigned int mc::DroppingSchedule::num_scheduled()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (the_only_buffer)
        return 1;
    else
        return 0;
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
