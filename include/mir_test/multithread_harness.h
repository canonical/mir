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
    Synchronizer() : kill(false) {}
    void set_kill() 
    {
        std::unique_lock<std::mutex> lk(sync_lock);
        kill = true;
        sync_cv.notify_all();
    }

    void set_data(T* in_data)
    {
        std::unique_lock<std::mutex> lk(sync_lock);
        data = in_data; 
        sync_cv.notify_all();
    }

    T* get_data()
    {
        std::unique_lock<std::mutex> lk(sync_lock);
        
        return data; 
    }

    bool sync()
    {
        std::unique_lock<std::mutex> lk(sync_lock);
        sync_cv.wait(lk);
        return kill;
    }

private:
    /* for signaling between threads */
    std::condition_variable sync_cv;

    /* for protecting data */
    std::mutex sync_lock;
    bool kill;
    T* data;
};


template <typename S, typename T>
void test_thread(std::function<void(std::shared_ptr<S>, Synchronizer<T>*)> function,
                       std::condition_variable *cv1, std::mutex* exec_lock, 
                       std::shared_ptr<S> test_object,
                       Synchronizer<T>* synchronizer)
{
    std::unique_lock<std::mutex> lk(*exec_lock);
    /* once we acquire lock, we know that the parent thread's timer is running, so we can release it.
        if its not released, then a deadlock situation would prevent the wait_until() from ever waking */
    lk.unlock();

    function(test_object, synchronizer);

    /* notify that we are done working */
    cv1->notify_one();
    return;
}

template <typename S, typename T>
void manager_thread(std::function<void(std::shared_ptr<S>, Synchronizer<T>*)> function,
                   std::shared_ptr<S> test_object , Synchronizer<T>* synchronizer,
                   std::chrono::milliseconds timeout)
{
    std::condition_variable cv1;
    std::mutex exec_lock;

    std::unique_lock<std::mutex> lk(exec_lock);
    std::thread(test_thread<S,T>, function, &cv1, &exec_lock, test_object, synchronizer).detach();

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

    synchronizer->set_kill();
    return;
}
}
}
#endif /* MIR_TEST_MULTITHREAD_HARNESS_ */
