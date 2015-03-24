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

#include <memory>
#include <thread>
#include "mir/fd.h"

namespace mir
{
namespace dispatch
{
class Dispatchable;


class SimpleDispatchThread
{
public:
    SimpleDispatchThread(std::shared_ptr<Dispatchable> const& dispatchee);
    SimpleDispatchThread(std::shared_ptr<Dispatchable> const& dispatchee,
                         std::function<void()> const& exception_handler);
    ~SimpleDispatchThread() noexcept;

private:
    Fd shutdown_fd;
    std::thread eventloop;
};

}
}


#endif // MIR_DISPATCH_SIMPLE_DISPATCH_THREAD_H_
