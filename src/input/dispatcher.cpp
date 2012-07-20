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

#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/event_handler.h"
#include "mir/input/filter.h"
#include "mir/input/logical_device.h"

#include <cassert>

namespace mi = mir::input;

mi::Dispatcher::Dispatcher(TimeSource* time_source, std::shared_ptr<Filter> const& filter_chain)
        : time_source(time_source),
          filter_chain(filter_chain)
{
    assert(time_source);
    assert(filter_chain);
}

// Implemented from EventHandler
void mi::Dispatcher::on_event(mi::Event* e)
{
    assert(e);

    std::lock_guard<std::mutex> lg(dispatcher_guard);

    e->set_system_timestamp(time_source->sample());

    filter_chain->accept(e);
}

#ifdef MIR_TODO_GXX44_FRIG
mi::Dispatcher::DeviceToken mi::Dispatcher::register_device(std::unique_ptr<mi::LogicalDevice> device)
{
    assert(device);
    auto pair = devices.insert(std::move(device));
    if (pair.second)
        (*pair.first)->start();

    return pair.first;
#else
mi::Dispatcher::DeviceToken mi::Dispatcher::register_device(std::unique_ptr<mi::LogicalDevice> /*device*/)
    {
    return devices.begin();
#endif
}

void mi::Dispatcher::unregister_device(mi::Dispatcher::DeviceToken token)
{
    (*token)->stop();
    devices.erase(token);
}

