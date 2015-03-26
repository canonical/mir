/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_THREAD_H_
#define MIR_ANDROID_UBUNTU_THREAD_H_

#include <std/Errors.h>
#include <std/ThreadDefs.h>
#include <std/RefBase.h>

#include <thread>
#include <atomic>
#include <stdexcept>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

namespace mir
{
void terminate_with_current_exception();
void set_thread_name(std::string const&);
}

namespace mir_input
{
class Thread : virtual public RefBase
{
public:
    // Create a Thread object, but doesn't create or start the associated
    // thread. See the run() method.
                        Thread(bool canCallJava = true) :
                            exit_pending(false), thread(), status(NO_ERROR) { (void)canCallJava; }
    virtual             ~Thread() { }

    // Start the thread in threadLoop() which needs to be implemented.
    virtual status_t    run(
        const char* name = 0,
        int32_t priority = PRIORITY_DEFAULT,
        size_t stack = 0)
    {
        (void)priority; (void)stack;
        std::string const name_str{name};

        status.store(NO_ERROR);
        exit_pending.store(false);

        // Avoid data races by doing a move capture instead of copy capture,
        // since libstdc++ is using a copy-on-write implementation of std::string
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=21334#c45
        thread = std::thread([name_str = std::move(name_str),this]
            {
                mir::set_thread_name(name_str);
                try
                {
                    if (auto result = readyToRun()) status.store(result);
                    else while (!exitPending() && threadLoop());
                }
                catch (...)
                {
                    mir::terminate_with_current_exception();
                }
            });

#ifdef HAVE_PTHREADS
        if (priority != PRIORITY_DEFAULT)
        {
            sched_param param;
            int policy;
            pthread_getschedparam(thread.native_handle(), &policy, &param);
            param.sched_priority = priority;
            pthread_setschedparam(thread.native_handle(), policy, &param);
        }
#endif

        return OK;
    }

    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void        requestExit() { exit_pending.store(true); }

    // Good place to do one-time initializations
    virtual status_t    readyToRun()  { return OK; }

    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
    virtual status_t    requestExitAndWait() { requestExit(); return join(); }

    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
            status_t    join() { if (thread.joinable()) thread.join(); return status.load(); }

protected:
    // exitPending() returns true if requestExit() has been called.
            bool        exitPending() const { return exit_pending.load(); }

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool        threadLoop() = 0;

private:
    std::atomic<bool> exit_pending;
    std::thread thread;
    std::atomic<status_t> status;
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
};
}
namespace android
{
using ::mir_input::Thread;
}
#endif /* MIR_ANDROID_UBUNTU_THREAD_H_ */
