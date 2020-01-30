/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_EVENT_FILTER_H_
#define MIR_INPUT_EVENT_FILTER_H_

#include "mir_toolkit/event.h"

namespace mir
{
namespace input
{

class EventFilter
{
public:
    virtual ~EventFilter() = default;

    // \return true indicates the event was consumed by the filter
    virtual bool handle(MirEvent const& event) = 0;

protected:
    EventFilter() = default;
    EventFilter(const EventFilter&) = delete;
    EventFilter& operator=(const EventFilter&) = delete;
};

}
}

#endif // MIR_INPUT_EVENT_FILTER_H_
