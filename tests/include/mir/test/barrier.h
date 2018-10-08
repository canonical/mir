/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_BARRIER_H_
#define MIR_TEST_BARRIER_H_

#include <mutex>
#include <condition_variable>

namespace mir
{
namespace test
{
class Barrier
{
public:
    explicit Barrier(unsigned wait_threads) : wait_threads{wait_threads} {}

    void reset(unsigned threads)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        wait_threads  = threads;
    }

    void ready()
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        --wait_threads;
        cv.notify_all();
        if (!cv.wait_for(lock, std::chrono::minutes(1), [&]{ return wait_threads == 0; }))
            throw std::runtime_error("Timeout");
    }

    Barrier(Barrier const&) = delete;
    Barrier& operator=(Barrier const&) = delete;
private:
    unsigned wait_threads;
    std::mutex mutex;
    std::condition_variable cv;
};
}
}



#endif /* MIR_TEST_BARRIER_H_ */
