/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

/** AutoUnblockThread is a helper thread class that can gracefully shutdown
 * at destruction time. This is helpul for tests that botch create
 * threads and use ASSERT macros for example (or any other condition that
 * makes the test exit early). Using naked std::thread would call std::terminate
 * under such conditions.
 */

#ifndef MIR_TEST_AUTO_UNBLOCK_THREAD_H_
#define MIR_TEST_AUTO_UNBLOCK_THREAD_H_

#include <thread>
#include <functional>

namespace mir
{
namespace test
{

class AutoUnblockThread
{
public:
    template<typename Callable, typename... Args>
    explicit AutoUnblockThread(std::function<void(void)> const& unblock,
        Callable&& f,
        Args&&... args)
        : unblock{unblock},
          thread{std::forward<Callable>(f), std::forward<Args>(args)...}
    {}

    ~AutoUnblockThread()
    {
        stop();
    }

    void stop()
    {
        unblock();
        if (thread.joinable())
            thread.join();
    }

    std::thread::native_handle_type native_handle()
    {
        return thread.native_handle();
    }

private:
    std::function<void(void)> unblock;
    std::thread thread;
};

/** AutoJoinThread is a convenience AutoUnblockThread when there is
 *  no need for unblocking anything to join the thread.
 */
class AutoJoinThread : public AutoUnblockThread
{
public:
    template<typename Callable, typename... Args>
    explicit AutoJoinThread(Callable&& f,
        Args&&... args)
        : AutoUnblockThread{[]{}, std::forward<Callable>(f), std::forward<Args>(args)...}
    {}
};

}
}
#endif
