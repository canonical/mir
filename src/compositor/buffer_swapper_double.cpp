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

#include "mir/compositor/buffer_swapper_double.h"
#include "mir/compositor/buffer.h"

namespace mc = mir::compositor;

mc::BufferSwapperDouble::BufferSwapperDouble(std::unique_ptr<Buffer> && buffer_a, std::unique_ptr<Buffer> && buffer_b)
    :
    buffer_a(std::move(buffer_a)),
    buffer_b(std::move(buffer_b)),
    invalid0(nullptr),
    invalid1(nullptr),
    wait_sync(false)
{
    atomic_store(&dequeued, &invalid0);
    atomic_store(&grabbed, &invalid1);
}

mc::Buffer* mc::BufferSwapperDouble::dequeue_free_buffer()
{
    while(!wait_sync) {
        std::unique_lock<std::mutex> lk(cv_mutex);
        cv.wait(lk); 
    }
    wait_sync = false;

    client_to_dequeued();
    return dequeued.load()->get();
}

void mc::BufferSwapperDouble::queue_finished_buffer()
{
    /* transition dequeued */
    client_to_queued();

    /* toggle grabbed pattern */
    compositor_change_toggle_pattern();
}

mc::Buffer* mc::BufferSwapperDouble::grab_last_posted()
{
    compositor_to_grabbed();
    return grabbed.load()->get();
}

void mc::BufferSwapperDouble::ungrab()
{
    compositor_to_ungrabbed();

    wait_sync = true;
    while (wait_sync) {
        cv.notify_all();
    }
}

/* class helper functions, mostly compare_and_exchange based state computation. 
   we need to compute next state of the atomics based solely on the current state, 
   consts in the class and variables local to the function. */
void mc::BufferSwapperDouble::client_to_dequeued()
{
    BufferPtr* dq_assume;
    BufferPtr* next_state;

    do
    {
        dq_assume = dequeued.load();
        next_state = dq_assume;

        if (dq_assume == &invalid0)
        {
            next_state = &buffer_a;
        }
        else if (dq_assume == &invalid1)
        {
            next_state = &buffer_b;
        }

    }
    while (!std::atomic_compare_exchange_weak(&dequeued, &dq_assume, next_state ));
}

void mc::BufferSwapperDouble::client_to_queued()
{
    BufferPtr* dq_assume;
    BufferPtr* next_state;

    do
    {
        dq_assume = dequeued.load();
        next_state = dq_assume;

        if (dq_assume == &buffer_a)
        {
            next_state = &invalid1;
        }
        else if (dq_assume == &buffer_b)
        {
            next_state = &invalid0;
        }

    }
    while (!std::atomic_compare_exchange_weak(&dequeued, &dq_assume, next_state ));
}

void mc::BufferSwapperDouble::compositor_change_toggle_pattern()
{
    BufferPtr* grabbed_assume;
    BufferPtr* next_state;

    do
    {
        grabbed_assume = grabbed.load();
        next_state = grabbed_assume;

        if (grabbed_assume == &invalid0)
        {
            next_state = &invalid1;
        }
        else if (grabbed_assume == &buffer_a)
        {
            next_state = &buffer_b;
        }

        else if (grabbed_assume == &invalid1)
        {
            next_state = &invalid0;
        }
        else if (grabbed_assume == &buffer_b)
        {
            next_state = &buffer_a;
        }
    }
    while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));
}

void mc::BufferSwapperDouble::compositor_to_grabbed()
{
    BufferPtr* grabbed_assume;
    BufferPtr* next_state;

    do
    {
        grabbed_assume = grabbed.load();
        next_state = grabbed_assume;

        if (grabbed_assume == &invalid0) /* pattern A */
        {
            next_state = &buffer_a;
        }
        else if (grabbed_assume == &invalid1) /* pattern B */
        {
            next_state = &buffer_b;
        }
    }
    while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));
}

void mc::BufferSwapperDouble::compositor_to_ungrabbed()
{
    BufferPtr* grabbed_assume;
    BufferPtr* next_state;
    do
    {
        grabbed_assume = grabbed.load();
        next_state = grabbed_assume;

        if (grabbed_assume == &buffer_a) /* pattern A */
        {
            next_state = &invalid0;
        }
        else if (grabbed_assume == &buffer_b) /* pattern B */
        {
            next_state = &invalid1;
        }
    }
    while (!std::atomic_compare_exchange_weak(&grabbed, &grabbed_assume, next_state ));
}

