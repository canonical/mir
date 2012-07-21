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

#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 4)
#include <cstdatomic>
#define MIR_USING_BOOST_THREADS
#else
#include <atomic>
#endif

#ifndef MIR_USING_BOOST_THREADS
#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#else
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
namespace mir
{
namespace std
{
using namespace ::std;
using ::boost::unique_future;
#define future unique_future
using ::boost::thread;
using ::boost::mutex;
using ::boost::condition_variable;
using ::boost::unique_lock;

enum class launch
{
    async,deferred,sync=deferred,any=async|deferred
};

template<typename Callable, typename... Args>
future<typename result_of<Callable(Args...)>::type>
async(launch policy, Callable&& functor, Args&&... args)
{
    typedef typename result_of<Callable(Args...)>::type result_type;
    ::boost::packaged_task<result_type> pt(functor, args...);
    future<result_type> result(pt.get_future());
    ::boost::thread(std::move(pt)).detach();
    return result;
}

}
}
#endif



#endif // MIR_THREAD_ALL_H_
