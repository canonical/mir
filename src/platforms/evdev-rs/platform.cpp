/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform.h"

#include "mir/dispatch/dispatchable.h"
#include "mir_platforms_evdev_rs/src/lib.rs.h"

namespace miers = mir::input::evdev_rs;
namespace md = mir::dispatch;

namespace
{
class DispatchableStub : public md::Dispatchable
{
public:
    mir::Fd watch_fd() const override
    {
        return mir::Fd{0};
    }

    bool dispatch(md::FdEvents) override
    {
        return false;
    }

    mir::dispatch::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }
};
}

std::shared_ptr<mir::dispatch::Dispatchable> miers::Platform::dispatchable()
{
    return std::make_shared<DispatchableStub>();
}

void miers::Platform::start()
{
    evdev_rs_start();
}

void miers::Platform::continue_after_config()
{
    evdev_rs_continue_after_config();
}

void miers::Platform::pause_for_config()
{
    evdev_rs_pause_for_config();
}

void miers::Platform::stop()
{
    evdev_rs_stop();
}
