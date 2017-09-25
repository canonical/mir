/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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

#ifndef MIR_CLIENT_EVENT_H_
#define MIR_CLIENT_EVENT_H_

#include "mir_toolkit/events/event.h"

#include <memory>

namespace mir
{
namespace client
{
/// Handle class for MirEvent - provides automatic reference counting.
class Event
{
public:
    Event() = default;
    explicit Event(MirEvent const* event) : self{mir_event_ref(event), deleter}
    {
    }

    operator MirEvent const*() const { return self.get(); }

    void reset() { self.reset(); }

private:
    static void deleter(MirEvent const* event) { mir_event_unref(event); }
    std::shared_ptr<MirEvent const> self;
};

// Provide a deleted overload to avoid double release "accidents".
void mir_event_unref(Event const& event) = delete;
}
}

#endif //MIR_CLIENT_EVENT_H_

