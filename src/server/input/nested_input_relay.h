/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_INPUT_NESTED_INPUT_RELAY_H_
#define MIR_INPUT_NESTED_INPUT_RELAY_H_

#include "mir/input/event_filter.h"

#include <memory>

namespace mir
{
namespace input
{
class InputDispatcher;
class NestedInputRelay : public EventFilter
{
public:
    NestedInputRelay();
    ~NestedInputRelay() noexcept;

    void set_dispatcher(std::shared_ptr<InputDispatcher> const& dispatcher);

private:
    bool handle(MirEvent const& event);

    std::shared_ptr<InputDispatcher> dispatcher;
};
}
}


#endif /* MIR_INPUT_NESTED_INPUT_RELAY_H_ */
