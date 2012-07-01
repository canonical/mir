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

mi::Dispatcher::Dispatcher(TimeSource* time_source,
                           std::unique_ptr<mi::Dispatcher::ShellFilter> shell,
                           std::unique_ptr<mi::Dispatcher::GrabFilter> grab,
                           std::unique_ptr<mi::Dispatcher::ApplicationFilter> application)
        : time_source(time_source),
          shell_filter(std::move(shell)),
          grab_filter(std::move(grab)),
          application_filter(std::move(application))
{
    assert(time_source);
    assert(shell_filter);
    assert(grab_filter);
    assert(application_filter);
}

// Implemented from EventHandler
void mi::Dispatcher::on_event(mi::Event* e)
{
    assert(e);

    e->set_system_timestamp(time_source->sample());
    
    if (shell_filter->accept(e) == mi::Filter::Result::stop_processing)
        return;

    if (grab_filter->accept(e) == mi::Filter::Result::stop_processing)
        return;

    application_filter->accept(e);
}

mi::Dispatcher::DeviceToken mi::Dispatcher::register_device(std::unique_ptr<mi::LogicalDevice> device)
{
    assert(device);
    auto pair = devices.insert(std::move(device));
    if (pair.second)
        (*pair.first)->start();

    return pair.first;
}

void mi::Dispatcher::unregister_device(mi::Dispatcher::DeviceToken token)
{    
    (*token)->stop();
    devices.erase(token);
}

