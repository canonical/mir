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

#include <condition_variable>
#include <mutex>
#include <gtest/gtest.h>

namespace mir
{
namespace testing
{

/* interface for main/controller thread to
   synchronize system */
class SynchronizerController
{
public:
    virtual ~SynchronizerController() {}

        virtual void ensure_child_is_waiting() = 0;
        virtual void activate_waiting_child() = 0;
        virtual void kill_thread() = 0;
};

/* interface for spawned threads to interact with main thread */
class SynchronizerSpawned
{
public:
    virtual ~SynchronizerSpawned() {}
    virtual bool child_enter_wait() = 0;
    virtual bool child_check_wait_request() = 0;
};

class Synchronizer : public SynchronizerController,
                     public SynchronizerSpawned
{
    public:
        Synchronizer ()
         : paused(false),
           pause_request(false),
           kill(false)
        {
        };

        ~Synchronizer ()
        {
        };

        void ensure_child_is_waiting()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            pause_request = true;
            while (!paused)
            {
                cv.wait(lk);
            }

            pause_request = false;
        };

        void activate_waiting_child()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            paused = false;
            cv.notify_all();
        };

        bool child_enter_wait()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            paused = true;
            cv.notify_all();
            while (paused) {
                cv.wait(lk);
            }

            return kill;
        };

        bool child_check_wait_request()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            return pause_request;
        };

        void kill_thread()
        {
            std::unique_lock<std::mutex> lk(sync_mutex);
            kill = true;
        };
    private:

        std::condition_variable cv;

        std::mutex sync_mutex;
        bool paused;
        bool pause_request;
        bool kill;
};

}
}
#endif /* MIR_TEST_MULTITHREAD_HARNESS_ */
