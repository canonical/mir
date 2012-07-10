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
    buf_a = a;
    buf_b = b;
    nullptr0 = nullptr;
    nullptr1 = nullptr;
    
    atomic_store(&dequeued, &nullptr0);
    atomic_store(&grabbed, &nullptr1);

}

void mc::BufferSwapperDouble::dequeue_free_buffer(Buffer*& out_buffer )
{
    Buffer **dq_assume;
    Buffer **next_state;

    do {
        dq_assume = dequeued.load();

        if (dq_assume == &nullptr0)
        {            
            next_state = &buf_a;
        } else if (dq_assume == &nullptr1)
        {
            next_state = &buf_b; 
        }
    } while (!std::atomic_compare_exchange_weak(&dequeued, &dq_assume, next_state )); 

    out_buffer = *dequeued.load();
}

void mc::BufferSwapperDouble::queue_finished_buffer(mc::Buffer*)
{
    Buffer **dq_assume;
    Buffer **grabbed_assume;
    Buffer **next_state;

    /* transition dequeued */
    do {
        dq_assume = dequeued.load();

        if (dq_assume == &buf_a)
        {
            next_state = &nullptr1; 
        }

        else
        if (dq_assume == &buf_b)
        {
            next_state = &nullptr0; 
        }

    } while (!std::atomic_compare_exchange_weak(&dequeued, &dq_assume, next_state ));


    /* toggle grabbed pattern */
    do {
        grabbed_assume = grabbed.load();

        if (grabbed_assume == &nullptr0)
        {
            next_state = &nullptr1; 
        }
        else
        if (grabbed_assume == &buf_a)
        {
            next_state = &buf_b; 
        }

        else
        if (grabbed_assume == &nullptr1)
        {
            next_state = &nullptr0; 
        }
        else
        if (grabbed_assume == &buf_b)
        { 
            next_state = &buf_a; 
        }
    } while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));

}

void mc::BufferSwapperDouble::toggle_to_grabbed() {
    Buffer **grabbed_assume;
    Buffer **next_state;

    do {
        grabbed_assume = grabbed.load();
        if (grabbed_assume == &nullptr0) /* pattern A */
        { 
            next_state = &buf_a; 
        }
        else 
        if (grabbed_assume == &nullptr1) /* pattern B */
        {
            next_state = &buf_b; 
        }
    } while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));
}

void mc::BufferSwapperDouble::toggle_to_ungrabbed() {
    Buffer **grabbed_assume;
    Buffer **next_state;
    /* must transition grabbed and last_posted */
    do {
        grabbed_assume = grabbed.load();
        if (grabbed_assume == &buf_a) /* pattern A */
        {
            
            next_state = &nullptr0; 
        }
        else
        if (grabbed_assume == &buf_b) /* pattern B */
        {
            
            next_state = &nullptr1; 
        }
    } while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));
}

void mc::BufferSwapperDouble::grab_last_posted(mc::Buffer*& out_buffer)
{
    toggle_to_grabbed();
    out_buffer = *grabbed.load();
}

void mc::BufferSwapperDouble::ungrab(mc::Buffer*)
{
    toggle_to_ungrabbed();
}
