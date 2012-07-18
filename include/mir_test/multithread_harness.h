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
                            std::function<void(SynchronizedThread<S,T>*, std::shared_ptr<S>, T* )> function,
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

}
}
#endif /* MIR_TEST_MULTITHREAD_HARNESS_ */
