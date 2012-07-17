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
#include <gtest/gtest.h>

namespace mir
{
namespace testing
{

template <typename T>
class Synchronizer {
    public:
        Synchronizer (int num_t)
         : num_threads(num_t),
           threads_waiting(0),
           threads_awake(0), 
           wait_ack(false),
           activate_ack(false),
           kill(false)
        {
            data = new T[num_t];
            for(int i=0; i<num_t; i++)
            {
                pause_requests.push_back(false);
            }

        };

        bool child_sync() 
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            threads_waiting++;
            if (threads_waiting == num_threads) {
                cv.notify_all();
            }

            while (!wait_ack) {
                cv.wait(lk);
            }

            threads_awake++;
            if(threads_awake == num_threads) 
            {
                cv.notify_all();
            }

            while (!activate_ack) {
                cv.wait(lk);
            }
    
            return kill;
        };



        void control_activate() 
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            for(int i=0; i<num_threads; i++) {
            /* note, change from orig */
            while(threads_awake<num_threads) { 
                cv.wait(lk);
            }
            }

            wait_ack = false;
            activate_ack = true;
            threads_waiting = 0;
            cv.notify_all();
        };


        bool child_check_pause(int tid)
        {
            std::unique_lock<std::mutex> lk(sync_mutex);

            if (pause_requests[tid] ) 
            {
                pause_requests[tid] = false;
                cv.notify_all();

                threads_waiting++;
                while (!wait_ack) {
                    cv.wait(lk);
                }

                threads_awake++;
                cv.notify_all();

                while (!activate_ack) {
                    cv.wait(lk);
                }
            }

            return kill;
        };

        void control_wait()
        { 
            std::unique_lock<std::mutex> lk(sync_mutex);
            while(threads_waiting != num_threads) { 
                cv.wait(lk);
            }

            threads_awake = 0;
            wait_ack = true;
            activate_ack = false;
            cv.notify_all();
        };

        void enforce_child_pause(int tid) 
        {
            std::unique_lock<std::mutex> lk(sync_mutex);

            pause_requests[tid] = true;

            while(pause_requests[tid]) {
                cv.wait(lk);
            }

        };

        void set_kill() {
            std::unique_lock<std::mutex> lk(sync_mutex);
            kill = true;
        
        }
        /* todo: thread id as index is very brittle system */
        void set_thread_data(T dat, int tid) {
            if ((tid < 0) || (tid > num_threads))
                return;
            data[tid] = dat;

        };

        T get_thread_data(int tid) 
        {
            if ((tid < 0) || (tid > num_threads))
                return nullptr;
            return data[tid];
        };

    private:
        const int num_threads;

        std::condition_variable cv;

        std::mutex sync_mutex;
        int threads_waiting;
        int threads_awake;
        bool wait_ack;
        bool activate_ack;

        std::vector<bool> pause_requests;
        bool kill;
        /* data in the Synchronizer */
        T* data;
};


template <typename S, typename T>
void test_thread(std::function<void(std::shared_ptr<S>, Synchronizer<T>*, int)> function,
                       std::condition_variable *cv1, std::mutex* exec_lock, 
                       std::shared_ptr<S> test_object, int tid ,
                       Synchronizer<T>* synchronizer)
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

template <typename S, typename T>
void manager_thread(std::function<void(std::shared_ptr<S>, Synchronizer<T>*, int)> function,
                   std::shared_ptr<S> test_object , int tid, Synchronizer<T>* synchronizer,
                   std::chrono::milliseconds timeout)
{
    std::condition_variable cv1;
    std::mutex exec_lock;

    std::unique_lock<std::mutex> lk(exec_lock);
    std::thread(test_thread<S,T>, function, &cv1, &exec_lock, test_object, tid, synchronizer).detach();

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
}
}
#endif /* MIR_TEST_MULTITHREAD_HARNESS_ */
