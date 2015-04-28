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

#ifndef MIR_DISPATCH_MULTIPLEXING_DISPATCHABLE_H_
#define MIR_DISPATCH_MULTIPLEXING_DISPATCHABLE_H_

#include "mir/dispatch/dispatchable.h"

#include <functional>
#include <initializer_list>
#include <list>
#include <mutex>

#include <pthread.h>

namespace mir
{
namespace dispatch
{
/**
 * \brief How concurrent dispatch should be handled
 */
enum class DispatchReentrancy
{
    sequential,     /**< The dispatch function is guaranteed not to be called
                     *   while a thread is currently running it.
                     */
    reentrant       /**< The dispatch function may be called on multiple threads
                     *   simultaneously
                     */
};

/**
 * \brief An adaptor that combines multiple Dispatchables into a single Dispatchable
 * \note Instances are fully thread-safe.
 */
class MultiplexingDispatchable final : public Dispatchable
{
public:
    MultiplexingDispatchable();
    MultiplexingDispatchable(std::initializer_list<std::shared_ptr<Dispatchable>> dispatchees);
    virtual ~MultiplexingDispatchable() noexcept;

    MultiplexingDispatchable& operator=(MultiplexingDispatchable const&) = delete;
    MultiplexingDispatchable(MultiplexingDispatchable const&) = delete;

    Fd watch_fd() const override;
    bool dispatch(FdEvents events) override;
    FdEvents relevant_events() const override;

    /**
     * \brief Add a dispatchable to the adaptor
     * \param [in] dispatchee   Dispatchable to add. The Dispatchable's dispatch()
     *                          function will not be called reentrantly.
     */
    void add_watch(std::shared_ptr<Dispatchable> const& dispatchee);
    /**
     * \brief Add a dispatchable to the adaptor, specifying the reentrancy of dispatch()
     */
    void add_watch(std::shared_ptr<Dispatchable> const& dispatchee, DispatchReentrancy reentrancy);

    /**
     * \brief Add a simple callback to the adaptor
     * \param [in] fd       File descriptor to monitor for readability
     * \param [in] callback Callback to fire when \ref fd becomes readable.
     *                      This callback is not called reentrantly.
     */
    void add_watch(Fd const& fd, std::function<void()> const& callback);

    /**
     * \brief Remove a watch from the dispatchable
     * \param [in] dispatchee   Dispatchable to remove
     */
    void remove_watch(std::shared_ptr<Dispatchable> const& dispatchee);

    /**
     * \brief Remove a watch by file-descriptor
     * \param [in] fd   File descriptor of watch to remove.
     */
    void remove_watch(Fd const& fd);
private:
    pthread_rwlock_t lifetime_mutex;
    std::list<std::pair<std::shared_ptr<Dispatchable>, bool>> dispatchee_holder;

    Fd epoll_fd;
};
}
}
#endif // MIR_DISPATCH_MULTIPLEXING_DISPATCHABLE_H_
