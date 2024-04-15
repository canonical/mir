/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SIGNAL_H_
#define MIR_SIGNAL_H_

#include <atomic>

namespace mir
{
/**
 * A thread-synchronisation barrier.
 *
 * The expected usage is that one thread sits in `wait()` until
 * signalled from another thread with `raise()`.
 *
 * This mechanism does not attempt to ensure that only one thread
 * is released per `raise()` or that each `raise()` unblocks only
 * one `wait()`. The only guarantees are that:
 * 1) At least one call to `raise()` strongly-happens-before 
 *    a call to `wait()` returns, and
 * 2) `wait()` will block until a call to `raise()` that 
 *    happens-after the most recent return from `wait()`.
 *
 * The primary use-case for such a barrier is to signal a
 * worker thread to run the next iteration of work.
 *
 * This is very similar to a `std::binary_semaphore` but
 * without the precondition that `raise()` is not called
 * if the Signal is already raised.
 */
class Signal
{
public:
    Signal();

    /**
     * Raise the signal, releasing any thread in `wait()`
     *
     * This does not synchronise with any other call to `raise()`
     * This synchronises-with a subsequent call to `wait()`, but does
     * not guarantee that each call to `raise()` releases at least one
     * `wait()`.
     *
     * Two unsynchronised calls to `raise()` may result in either one
     * or two calls to `wait()` being unblocked, depending on timing.
     */
    void raise();

    /**
     * Wait for the signal to be raised
     *
     * This synchronises-wtih a previous call to `raise()`, and
     * then resets the signal.
     */
    void wait();
private:
    std::atomic<bool> flag;
};
}

#endif // MIR_SIGNAL_H_
