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
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

mc::DroppingSchedule::DroppingSchedule(std::shared_ptr<mf::ClientBuffers> const&)
{
}

void mc::DroppingSchedule::schedule(std::shared_ptr<mg::Buffer> const& buffer)
{
    (void) buffer;
}

void mc::DroppingSchedule::cancel(std::shared_ptr<mg::Buffer> const& buffer)
{
    (void) buffer;
}

bool mc::DroppingSchedule::anything_scheduled()
{
    return false;
}

std::shared_ptr<mg::Buffer> mc::DroppingSchedule::next_buffer()
{
    return nullptr;
}
