/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_THREAD_POOL_EXECUTOR_H_
#define MIR_THREAD_POOL_EXECUTOR_H_

#include "mir/executor.h"

namespace mir
{
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
}

#endif // MIR_THREAD_POOL_EXECUTOR_H_
