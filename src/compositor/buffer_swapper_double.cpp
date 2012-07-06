/*
 * Copyright © 2012 Canonical Ltd.
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
#include <mir/compositor/buffer_swapper_double.h>

namespace mc = mir::compositor;

mc::BufferSwapperDouble::BufferSwapperDouble(std::shared_ptr<Buffer> , std::shared_ptr<Buffer> )
{
}

void mc::BufferSwapperDouble::dequeue_free_buffer(std::shared_ptr<Buffer>& )
{
}

void mc::BufferSwapperDouble::queue_finished_buffer(std::shared_ptr<Buffer>& )
{
}

void mc::BufferSwapperDouble::grab_last_posted(std::shared_ptr<Buffer>& )
{
}

void mc::BufferSwapperDouble::ungrab(std::shared_ptr<Buffer>&  )
{
}
