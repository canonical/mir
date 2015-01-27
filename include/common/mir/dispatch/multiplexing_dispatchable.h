/*
 * Copyright © 2014 Canonical Ltd.
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
#include <tuple>

namespace mir
{
namespace dispatch
{
class MultiplexingDispatchable : public Dispatchable
{
public:
    MultiplexingDispatchable();
    MultiplexingDispatchable(std::initializer_list<std::shared_ptr<Dispatchable>> dispatchees);
    virtual ~MultiplexingDispatchable() = default;

    MultiplexingDispatchable& operator=(MultiplexingDispatchable const&) = delete;
    MultiplexingDispatchable(MultiplexingDispatchable const&) = delete;

    Fd watch_fd() const override;
    bool dispatch(FdEvents events) override;
    FdEvents relevant_events() const override;

    void add_watch(std::shared_ptr<Dispatchable> const& dispatchee);
    void remove_watch(std::shared_ptr<Dispatchable> const& dispatchee);
private:
    std::mutex lifetime_mutex;
    std::list<std::shared_ptr<Dispatchable>> dispatchee_holder;

    Fd epoll_fd;
};
}
}
#endif // MIR_DISPATCH_MULTIPLEXING_DISPATCHABLE_H_
