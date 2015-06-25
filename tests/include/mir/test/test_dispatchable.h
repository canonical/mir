/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */


#ifndef MIR_TEST_TEST_DISPATCHABLE_H_
#define MIR_TEST_TEST_DISPATCHABLE_H_

#include "mir/test/pipe.h"
#include "mir/dispatch/dispatchable.h"

#include <functional>

namespace mir
{
namespace test
{

class TestDispatchable : public dispatch::Dispatchable
{
public:
    TestDispatchable(std::function<bool(dispatch::FdEvents)> const& target,
                     dispatch::FdEvents relevant_events);
    TestDispatchable(std::function<bool(dispatch::FdEvents)> const& target);
    TestDispatchable(std::function<void()> const& target);

    mir::Fd watch_fd() const override;
    bool dispatch(dispatch::FdEvents events) override;
    dispatch::FdEvents relevant_events() const override;

    void trigger();
    void untrigger();
    void hangup();

private:
    mir::Fd read_fd;
    mir::Fd write_fd;
    std::function<bool(dispatch::FdEvents)> const target;
    dispatch::FdEvents const eventmask;
};

}
}


#endif // MIR_TEST_TEST_DISPATCHABLE_H_
