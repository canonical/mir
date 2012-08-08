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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_THREAD_ALL_H_
#define MIR_THREAD_ALL_H_

#include "mir/chrono/all.h"

#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 4)
#include <cstdatomic>
// For std::this_thread::sleep_for etc.
#define _GLIBCXX_USE_NANOSLEEP
#define MIR_USING_BOOST_THREADS
#else
#include <atomic>
#endif

#include <thread>
#include <mutex>

#ifndef MIR_USING_BOOST_THREADS
#include <future>
#include <condition_variable>
#include <mutex>
#else
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/condition_variable.hpp>
namespace mir
{
namespace std
{
using namespace ::std;
using ::boost::unique_future;
using ::boost::unique_lock;
using ::boost::condition_variable;
using ::boost::lock_guard;
#define future unique_future
using ::boost::thread;
using ::boost::mutex;

enum class launch
{
    async,deferred,sync=deferred,any=async|deferred
};

template<typename Callable, typename... Args>
future<typename result_of<Callable(Args...)>::type>
async(launch /*policy*/, Callable&& functor, Args&&... args)
{
    typedef typename result_of<Callable(Args...)>::type result_type;
    ::boost::packaged_task<result_type> pt(functor, args...);
    future<result_type> result(pt.get_future());
    ::boost::thread(std::move(pt)).detach();
    return result;
}

template <class UnderlyingType>
inline bool atomic_compare_exchange_weak(atomic<UnderlyingType*>* target, UnderlyingType** compare, UnderlyingType* const& next_state)
{
    return atomic_compare_exchange_weak(target, (void**)compare, next_state);
}

namespace cv_status
{
bool const no_timeout = true;
bool const timeout = false;
}

namespace this_thread
{
using namespace ::boost::this_thread;

template <class Timepoint>
void sleep_until(const Timepoint& abs_time)
{
    sleep(abs_time);
}

template <class Duration>
void sleep_for(const Duration& rel_time)
{
    sleep(rel_time);
}

}
}
}
#endif

#endif // MIR_THREAD_ALL_H_
