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

#ifndef EVENT_H_
#define EVENT_H_

#include "mir/time_source.h"

namespace mir
{
namespace input
{

class Event {
 public:
    
    virtual ~Event() {}
    
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    
    // The system timestamp as assigned to the event
    // when entering the event processing.
    const mir::Timestamp& SystemTimestamp() const
    {
        return system_timestamp;
    }
    
    void SetSystemTimestamp(const mir::Timestamp& ts)
    {
        system_timestamp = ts;
    }

 protected:
    Event() = default;
 private:
    mir::Timestamp system_timestamp;
};
    
}}

#endif // EVENT_H_
