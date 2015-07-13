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

#include "timeout_queue.h"
namespace mc = mir::compositor;
namespace mg = mir::graphics;

void mc::TimeoutQueue::schedule(std::shared_ptr<graphics::Buffer> const& buffer)
{
    (void) buffer;
}

void mc::TimeoutQueue::cancel(std::shared_ptr<graphics::Buffer> const& buffer)
{
    (void) buffer;
}

bool mc::TimeoutQueue::anything_scheduled()
{
    return false;
}

std::shared_ptr<mg::Buffer> mc::TimeoutQueue::next_buffer()
{
    return nullptr;
}
