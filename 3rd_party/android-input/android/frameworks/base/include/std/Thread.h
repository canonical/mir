/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_THREAD_H_
#define MIR_ANDROID_UBUNTU_THREAD_H_

#include ANDROIDFW_UTILS(Errors.h)
#include ANDROIDFW_UTILS(ThreadDefs.h)
#include ANDROIDFW_UTILS(RefBase.h)

#include <thread>
#include <stdexcept>

namespace android
{
class Thread : virtual public RefBase
{
public:
    // Create a Thread object, but doesn't create or start the associated
    // thread. See the run() method.
                        Thread(bool canCallJava = true) { (void)canCallJava; throw std::logic_error("not implemented"); }
    virtual             ~Thread() { throw std::logic_error("not implemented"); }

    // Start the thread in threadLoop() which needs to be implemented.
    virtual status_t    run(    const char* name = 0,
                                int32_t priority = PRIORITY_DEFAULT,
                                size_t stack = 0) { (void)name; (void)priority; (void)stack; throw std::logic_error("not implemented"); }

    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void        requestExit()  { throw std::logic_error("not implemented"); }

//    // Good place to do one-time initializations
//    virtual status_t    readyToRun()  { throw std::logic_error("not implemented"); }

    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
    virtual status_t    requestExitAndWait()  { throw std::logic_error("not implemented"); }

    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
            status_t    join()  { throw std::logic_error("not implemented"); }
//
//protected:
//    // exitPending() returns true if requestExit() has been called.
//            bool        exitPending() const;

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool        threadLoop() = 0;

private:
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
};
}

#endif /* MIR_ANDROID_UBUNTU_THREAD_H_ */
