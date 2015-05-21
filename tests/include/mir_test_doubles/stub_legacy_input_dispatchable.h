/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny
 */

#ifndef MIR_TEST_DOUBLES_STUB_LEGACY_INPUT_DISPATCHABLE_H_
#define MIR_TEST_DOUBLES_STUB_LEGACY_INPUT_DISPATCHABLE_H_

#include "mir/input/legacy_input_dispatchable.h"
#include "mir/dispatch/action_queue.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct StubLegacyInputDispatchable : input::LegacyInputDispatchable
{
    void start() override
    {}
    Fd watch_fd() const override
    {
        return queue.watch_fd();
    }
    bool dispatch(dispatch::FdEvents events) override
    {
        return queue.dispatch(events);
    }
    dispatch::FdEvents relevant_events() const override
    {
        return queue.relevant_events();
    }
    mir::dispatch::ActionQueue queue;
};
}
}
}
#endif
