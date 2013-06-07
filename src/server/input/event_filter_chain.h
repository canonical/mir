/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_EVENT_FILTER_CHAIN_H_
#define MIR_INPUT_EVENT_FILTER_CHAIN_H_

#include <memory>
#include <vector>

#include "mir/input/event_filter.h"

namespace android
{
class InputEvent;
}

namespace droidinput = android;

namespace mir
{
namespace input
{

class EventFilterChain : public EventFilter
{
public:
    explicit EventFilterChain(std::initializer_list<std::shared_ptr<EventFilter> const> values);

    virtual bool handle(const MirEvent &event);

private:
    typedef std::vector<std::weak_ptr<EventFilter>> EventFilterVector;
    EventFilterVector filters;
};

}
}

#endif // MIR_INPUT_EVENT_FILTER_H_
