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
    
    atomic_store(&on_deck, &buf_a);
    atomic_store(&last_posted, &buf_b);
    atomic_store(&dequeued, &nullptr0);
    atomic_store(&grabbed, &nullptr1);

}

void mc::BufferSwapperDouble::dequeue_free_buffer(Buffer*& out_buffer )
{
    Buffer **dq_assume;
    Buffer **next_state;
    /* dequeue buffer must, in this order:
     * 1) transition the on_deck to the next state
     * 2) transition the dequeued to the next state */

    /* step 2 */
    do {
        dq_assume = dequeued.load();

        if (dq_assume == &nullptr0)
        {    
            
            next_state = &buf_a;
        } else if (dq_assume == &buf_a)
        {
            
            next_state = &nullptr1; 
        } else if (dq_assume == &nullptr1)
        {
            
            next_state = &buf_b; 
        } else if (dq_assume == &buf_b)
        {
            
            next_state = &nullptr0; 
        }

    } while (!std::atomic_compare_exchange_weak(&dequeued, &dq_assume, next_state ));
    

    out_buffer = *dequeued.load();
}

void mc::BufferSwapperDouble::queue_finished_buffer(mc::Buffer*)
{
    Buffer **dq_assume;
//    Buffer **on_deck_assume;
    Buffer **last_posted_assume;
    Buffer **next_state;
    /* queue buffer must do these things in this order:
    * 1) transition on_deck
    * 2) transition last_posted
    * 3) transition dequeued */

    /* transition last_posted */
    do {
        last_posted_assume = last_posted.load();
        /* note: if last_posted_assume is an invalid value, this is an error */

        if (*last_posted_assume == buf_a)
        {
           next_state = &nullptr1;
        }
        if (*last_posted_assume == buf_b)
        {
            next_state = &nullptr0;
        }

    } while (!std::atomic_compare_exchange_weak(&last_posted, &last_posted_assume, next_state ));

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
}

void mc::BufferSwapperDouble::grab_last_posted(mc::Buffer*& out_buffer)
{
    Buffer **grabbed_assume;
    Buffer **next_state;

    /* must transition grabbed and last_posted */
    do {
        grabbed_assume = grabbed.load();

        if (grabbed_assume == &buf_a)
        {
            
            next_state = &nullptr1; 
        }

        else
        if (grabbed_assume == &buf_b)
        {
            
            next_state = &nullptr0; 
        }

    } while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));

    out_buffer = *grabbed.load();


}

void mc::BufferSwapperDouble::ungrab(mc::Buffer*)
{

}
