/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_INPUT_EVENT_HANDLER_H_
#define MIR_INPUT_EVENT_HANDLER_H_

namespace mir
{
namespace input
{

class Event;

// EventHandler is the interface by which input
// devices know the input dispatcher
class EventHandler
{
 public:
    virtual ~EventHandler() {}

    virtual void on_event(Event* e) = 0;
protected:
    EventHandler() = default;
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
};

}
}

#endif /* MIR_INPUT_EVENT_HANDLER_H_ */
