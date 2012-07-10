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

#include <memory>
#include <atomic>

mc::BufferSwapperDouble::BufferSwapperDouble(std::shared_ptr<Buffer> a, std::shared_ptr<Buffer> b )
{
    atomic_store(&on_deck, a.get());
    atomic_store(&last_posted, b.get());

    mc::Buffer *tmp = nullptr;
    atomic_store(&dequeued, tmp);
    atomic_store(&grabbed, tmp);

    atomic_store(&new_last_posted, false);

}

void mc::BufferSwapperDouble::atomic_swap(std::atomic<mc::Buffer*>& a, std::atomic<mc::Buffer*>& b)
{
    mc::Buffer *tmp, *tmp2;
    do {
        tmp = atomic_load(&a);
        do {
            tmp2 = atomic_load(&b);
        } while (!std::atomic_compare_exchange_weak(&a, &tmp, tmp2));
    } while (!std::atomic_compare_exchange_weak(&b, &tmp2, tmp ));
}

void mc::BufferSwapperDouble::dequeue_free_buffer(Buffer*& out_buffer)
{
    atomic_swap(dequeued, on_deck);

    /* the algorithm ensures that once the dequeued Buffer* is filled, it is essentially
        'locked' until queue_finished_buffer is called with this handle */
    out_buffer = atomic_load(&dequeued);     
}

void mc::BufferSwapperDouble::queue_finished_buffer(mc::Buffer* )
{
    atomic_swap(on_deck, dequeued);
    atomic_swap(last_posted, on_deck);
}

void mc::BufferSwapperDouble::grab_last_posted(mc::Buffer*&)
{
}

void mc::BufferSwapperDouble::ungrab(mc::Buffer*  )
{
}
