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

#ifndef MIR_INPUT_DISPATCHER_H_
#define MIR_INPUT_DISPATCHER_H_

#include "mir/input/event_handler.h"
#include "mir/input/filter.h"

#include "mir/thread/all.h"

#include <memory>
#include <set>

#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 4)
#define MIR_WORKAROUND_SET_NOT_SUPPORTING_MOVEONLY_TYPES
#endif

namespace mir
{

class TimeSource;

namespace input
{

class Event;
class LogicalDevice;

class Dispatcher : public EventHandler
{
#ifndef MIR_WORKAROUND_SET_NOT_SUPPORTING_MOVEONLY_TYPES
    typedef std::set< std::unique_ptr<LogicalDevice> > DeviceCollection;
#else
    struct Frig : std::unique_ptr<LogicalDevice>
    {
        typedef std::unique_ptr<LogicalDevice> base;
        Frig(base&& sp);
        Frig(Frig const& f);
        ~Frig();
    };

    typedef std::set< Frig > DeviceCollection;
#endif

 public:
    typedef DeviceCollection::iterator DeviceToken;

    Dispatcher(TimeSource* time_source, std::shared_ptr<Filter> const& filter_chain);
    // Implemented from EventHandler
    void on_event(Event* e);

    DeviceToken register_device(std::unique_ptr<LogicalDevice> device);

    void unregister_device(DeviceToken token);

 private:
    TimeSource* time_source;

    std::shared_ptr<Filter> filter_chain;

    DeviceCollection devices;
    std::mutex dispatcher_guard;
};

}
}

#endif /* MIR_INPUT_DISPATCHER_H_ */
