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

#include <cassert>

namespace mi = mir::input;

mi::Dispatcher::Dispatcher(TimeSource* time_source,
                           mi::Filter* shell_filter,
                           mi::Filter* grab_filter,
                           mi::Filter* application_filter)
        : time_source(time_source),
          shell_filter(shell_filter),
          grab_filter(grab_filter),
          application_filter(application_filter)
{
    assert(time_source);
    assert(shell_filter);
    assert(grab_filter);
    assert(application_filter);
}

// Implemented from EventHandler
void mi::Dispatcher::OnEvent(mi::Event* e)
{
    if (!e)
        return;

    e->SetSystemTimestamp(time_source->Sample());
    
    if (shell_filter->Accept(e))
        return;

    if (grab_filter->Accept(e))
        return;

    application_filter->Accept(e);
}

void mi::Dispatcher::RegisterShellFilter(mi::Filter* f)
{
    assert(f);
    shell_filter = f;
}

void mi::Dispatcher::RegisterGrabFilter(mi::Filter* f)
{
    assert(f);
    grab_filter = f;
}

void mi::Dispatcher::RegisterApplicationFilter(mi::Filter* f)
{
    assert(f);
    application_filter = f;
}
