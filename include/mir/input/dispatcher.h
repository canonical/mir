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

#include <set>

namespace mir
{

class TimeSource;

namespace input
{

class Event;
class Filter;
class LogicalDevice;

class Dispatcher : public EventHandler
{
 public:

    Dispatcher(TimeSource* time_source,
               Filter* shell_filter,
               Filter* grab_filter,
               Filter* application_filter);

    ~Dispatcher() = default;

    // Implemented from EventHandler
    void on_event(Event* e);

    void RegisterShellFilter(Filter* f);

    void RegisterGrabFilter(Filter* f);

    void RegisterApplicationFilter(Filter* f);

    void register_device(LogicalDevice* device);

    void unregister_device(LogicalDevice* device);
 private:
    TimeSource* time_source;
    
    Filter* shell_filter;
    Filter* grab_filter;
    Filter* application_filter;

    std::set<LogicalDevice*> devices;
};

}
}

#endif /* MIR_INPUT_DISPATCHER_H_ */
