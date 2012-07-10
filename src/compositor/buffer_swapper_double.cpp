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

mc::BufferSwapperDouble::BufferSwapperDouble(mc::Buffer* a, mc::Buffer* b )
{
    atomic_store(&on_deck, a);
    atomic_store(&last_posted, b);

    mc::Buffer *tmp = nullptr;
    atomic_store(&dequeued, tmp);
    atomic_store(&grabbed, tmp);
    atomic_store(&new_last_posted, false);

}



#include <iostream>
void mc::BufferSwapperDouble::lockless_swap(std::atomic<mc::Buffer*>& a,
std::atomic<mc::Buffer*>& b)
{
    std::unique_lock<std::mutex> lk(state_mutex);
    Buffer* tmp;
    tmp = a.load();
    a.store(b.load());
    b.store(tmp);
    
}

void mc::BufferSwapperDouble::dequeue_free_buffer(Buffer*& out_buffer)
{
    lockless_swap(on_deck, dequeued);

    out_buffer = dequeued.load();
    /* this should be atomic */

}

void mc::BufferSwapperDouble::queue_finished_buffer(mc::Buffer*)
{
    lockless_swap(on_deck, last_posted);
    lockless_swap(last_posted, dequeued);

}

void mc::BufferSwapperDouble::grab_last_posted(mc::Buffer*& out_buffer)
{
    lockless_swap(grabbed, last_posted);
    
    out_buffer = grabbed.load();
}

void mc::BufferSwapperDouble::ungrab(mc::Buffer*)
{
}
