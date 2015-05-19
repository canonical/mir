/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_DISPATCH_SIMPLE_DISPATCH_THREAD_H_
#define MIR_DISPATCH_SIMPLE_DISPATCH_THREAD_H_

#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/fd.h"

namespace mir
{
namespace dispatch
{
class Dispatchable;

class ThreadedDispatcher
{
public:
    ThreadedDispatcher(std::string const& name, std::shared_ptr<Dispatchable> const& dispatchee);
    ThreadedDispatcher(std::string const& name,
                       std::shared_ptr<Dispatchable> const& dispatchee,
                       std::function<void()> const& exception_handler);
    ~ThreadedDispatcher() noexcept;

    void add_thread();
    void remove_thread();

    class ThreadShutdownRequestHandler;
private:

    std::string const name_base;

    std::shared_ptr<ThreadShutdownRequestHandler> thread_exiter;
    std::shared_ptr<MultiplexingDispatchable> dispatcher;

    std::mutex thread_pool_mutex;
    std::vector<std::thread> threadpool;

    std::function<void()> const exception_handler;
};

}
}


#endif // MIR_DISPATCH_SIMPLE_DISPATCH_THREAD_H_
