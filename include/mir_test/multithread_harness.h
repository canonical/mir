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
#ifndef MIR_TEST_MULTITHREAD_HARNESS_H_
#define MIR_TEST_MULTITHREAD_HARNESS_H_

#include <thread>
#include <chrono>
#include <gtest/gtest.h>

namespace mir
{
namespace testing
{

template< typename S, typename T>
class SynchronizedThread {
    public:
        SynchronizedThread (std::chrono::time_point<std::chrono::system_clock> timeout,
                            std::function<void(SynchronizedThread*, std::shared_ptr<S>, T* )> function,
                            std::shared_ptr<S> test_object,
                            T* data )
         : abs_timeout(timeout),
           paused(false),
           pause_request(false),
           kill(false)
        {
            thread = new std::thread(function, this, test_object, data);
        };

        ~SynchronizedThread ()
        {
            thread->join();
            delete thread;
        };

        /* called from main thread, must time out */
        void stabilize() 
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            pause_request = true;
            if (!paused) 
            {
                if (std::cv_status::timeout == cv.wait_until(lk, abs_timeout))
                {
                    FAIL();
                    return;
                }
            }
            pause_request = false;
        };
    
        void activate() 
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            paused = false;
            cv.notify_all();
        };

        bool child_wait()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            paused = true;
            cv.notify_all();
            while (paused) {
                cv.wait_for(lk, std::chrono::milliseconds(250));
            }

            return kill;
        };

        bool child_check()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            if (pause_request)
            {
                paused = true;
                cv.notify_all();
                while (paused) {
                    cv.wait_for(lk, std::chrono::milliseconds(250));
                }
            }
    
            return kill;
        };
        
        void kill_thread()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            kill = true;
        };
    private:

        std::chrono::time_point<std::chrono::system_clock> abs_timeout;

        std::thread *thread;
        std::condition_variable cv;

        std::mutex sync_mutex;
        bool paused;
        bool pause_request;
        bool kill;
};

#if 0 
template <typename S>
void test_thread(std::function<void(std::shared_ptr<S>, Synchronizer*, int)> function,
                       std::condition_variable *cv1, std::mutex* exec_lock, 
                       std::shared_ptr<S> test_object, int tid ,
                       Synchronizer* synchronizer)
{
    std::unique_lock<std::mutex> lk(*exec_lock);
    /* once we acquire lock, we know that the parent thread's timer is running, so we can release it.
        if its not released, then a deadlock situation would prevent the wait_until() from ever waking */
    lk.unlock();

    function(test_object, synchronizer, tid);

    /* notify that we are done working */
    cv1->notify_one();
    return;
}

template <typename S>
void manager_thread(std::function<void(std::shared_ptr<S>, Synchronizer*, int)> function,
                   std::shared_ptr<S> test_object , int tid, Synchronizer* synchronizer,
                   std::chrono::milliseconds timeout)
{
    std::condition_variable cv1;
    std::mutex exec_lock;

    std::unique_lock<std::mutex> lk(exec_lock);
    std::thread(test_thread<S>, function, &cv1, &exec_lock, test_object, tid, synchronizer).detach();

    /* set timeout as absolute time from this point */
    auto timeout_time = std::chrono::system_clock::now() + timeout;

    /* once the cv starts waiting, it releases the lock and triggers the timeout-protected 
       thread that it can start the execution of the timeout function. this way the tiemout
       thread cannot send a cv notify_one before we are actually waiting (as with a very short
       target function */
    if (std::cv_status::timeout == cv1.wait_until(lk, timeout_time ))
    {
        FAIL();
    }

    return;
}
#endif
}
}
#endif /* MIR_TEST_MULTITHREAD_HARNESS_ */
