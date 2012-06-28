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

#ifndef MIR_INPUT_EVENT_PRODUCER_H_
#define MIR_INPUT_EVENT_PRODUCER_H_

#include <cassert>

namespace mir
{
namespace input
{

class EventHandler;

class EventProducer
{
 public:

    enum class State
    {
        running,
        stopped
    };
    
    explicit EventProducer(EventHandler* handler) : event_handler(handler)
    {
        assert(handler);
    }
    
    virtual ~EventProducer() {}

    virtual State get_state() const = 0;

    // Starts execution of the event producer.
    // Implementations have to be non-blocking.
    virtual void start() = 0;

    // Stops execution of the event producer.
    // Implementations have to block until no
    // more event will be propagated to the event_handler.
    virtual void stop() = 0;
    
 protected:
    EventProducer(const EventProducer&) = delete;
    EventProducer& operator=(const EventProducer&) = delete;
    
    EventHandler* event_handler;
};

}
}

#endif // MIR_INPUT_EVENT_PRODUCER_H_
