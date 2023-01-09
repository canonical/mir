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

#ifndef MIR_EXECUTOR_H_
#define MIR_EXECUTOR_H_

#include <functional>

namespace mir
{

/**
 * An executor abstraction mostly compatible with C++ proposal N4414
 *
 * As specified in http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0008r0.pdf
 *
 * This should hopefully be compatible with whatever the C++ Executors and Schedulers group
 * come up with as a final design, and will eventually be replaced by the standard version.
 *
 * If not, this minimal interface should be easy to implement on top of whatever emerges
 * from the standards body.
 */
class Executor
{
public:
    /**
     * Schedule some function to be called sometime in the future.
     *
     * It is expected that this \b not eagerly execute, and will return
     * \b before work() is executed.
     *
     * \param [in] work     Function to execute in Executor specified environment.
     */
    virtual void spawn(std::function<void()>&& work) = 0;

protected:
    virtual ~Executor() = default;
};

/**
 * An executor which never blocks the spawn() method on work completing.
 */
class NonBlockingExecutor: public Executor
{
};

class ThreadPoolExecutor : public NonBlockingExecutor
{
public:
    void spawn(std::function<void()>&& work) override;

    /**
     * Set a handler to be called should an unhandled exception occur on the ThreadPoolExecutor
     *
     * By default the ThreadPoolExecutor will allow exceptions to propagate, resulting in process termination.
     *
     * \param handler [in]  A handler that will be called in exception context should an exception escape
     *                      any functor executed by \ref spawn.
     *                      This handler will be called on an unspecified thread.
     */
    static void set_unhandled_exception_handler(void (*handler)());

    /**
     * Wait for all current work to finish and terminate all worker threads
     */
    static void quiesce();
protected:
    ThreadPoolExecutor() = default;
};

extern NonBlockingExecutor& thread_pool_executor;

/**
 * An Executor that makes the following concurrency guarantees:
 *
 * 1.   If linearising_executor.spawn(A) happens-before linearising_executor.spawn(B) then
 *      A() happens-before B(). That is, the entire execution of A is completed before B is started.
 * 2.   No work is performed concurrently. For any two calls linearising_executor.spawn(A)
 *      linearising_executor.spawn(B) either A() happens-before B() or B() happens-before A().
 * 3.   Work is deferred; linearising_executor.spawn(A) will not block on the execution of A
 */
extern NonBlockingExecutor& linearising_executor;

/**
 * An Executor that runs work on the current thread within spawn()
 */
extern Executor& immediate_executor;
}

#endif // MIR_EXECUTOR_H_
