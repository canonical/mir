/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** AutoUnblockThread is a helper thread class that can gracefully shutdown
 * at destruction time. This is helpul for tests that botch create
 * threads and use ASSERT macros for example (or any other condition that
 * makes the test exit early). Using naked std::thread would call std::terminate
 * under such conditions.
 */

#ifndef MIR_TEST_AUTO_UNBLOCK_THREAD_H_
#define MIR_TEST_AUTO_UNBLOCK_THREAD_H_

#include <functional>
#include <memory>
#include <stop_token>
#include <thread>

namespace mir
{
namespace test
{
class AutoUnblockThread
{
public:
    AutoUnblockThread() = default;

    template<typename Callable, typename... Args>
    explicit AutoUnblockThread(std::function<void(void)> unblock, Callable&& f, Args&&... args) :
        jthread{std::forward<Callable>(f), std::forward<Args>(args)...},
        unblock_on_stop{std::make_unique<UnblockCallback>(jthread.get_stop_token(), std::move(unblock))}
    {}

    ~AutoUnblockThread() { stop(); }

    AutoUnblockThread(AutoUnblockThread&& t) = default;
    AutoUnblockThread& operator=(AutoUnblockThread&& t) = default;

    void stop()
    {
        jthread.request_stop();
        if (jthread.joinable())
            jthread.join();
    }

private:
    using UnblockCallback = std::stop_callback<std::function<void(void)>>;

    std::jthread jthread;
    std::unique_ptr<UnblockCallback> unblock_on_stop;
};

}
}
#endif
