/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "nested_input_relay.h"

#include "mir/input/input_dispatcher.h"


namespace mi = mir::input;

mi::NestedInputRelay::NestedInputRelay()
{
}

mi::NestedInputRelay::~NestedInputRelay() noexcept
{
}

void mi::NestedInputRelay::set_dispatcher(std::shared_ptr<InputDispatcher> const& dispatcher)
{
    this->dispatcher = dispatcher;
}

bool mi::NestedInputRelay::handle(MirEvent const& event)
{
    if (dispatcher == nullptr)
    {
        return false;
    }

    if (event.type == mir_event_type_motion ||
        event.type == mir_event_type_key)
        dispatcher->dispatch(event);

    return true;
}
