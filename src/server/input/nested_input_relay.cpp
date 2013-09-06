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

#include "mir/input/nested_input_relay.h"

#include <InputDispatcher.h>

namespace mi = mir::input;

mi::NestedInputRelay::NestedInputRelay()
{
}

mi::NestedInputRelay::~NestedInputRelay() noexcept
{
}

void mi::NestedInputRelay::set_dispatcher(::android::sp<::android::InputDispatcher> const& dispatcher)
{
    this->dispatcher = dispatcher;
}

bool mi::NestedInputRelay::handle(MirEvent const& /*event*/)
{
    if (dispatcher == nullptr)
    {
        return false;
    }

    // TODO convert event to an InputEvent
//    dispatcher->injectInputEvent(
//        const InputEvent* event,
//        int32_t injectorPid,
//        int32_t injectorUid,
//        int32_t syncMode,
//        int32_t timeoutMillis,
//        uint32_t policyFlags);
    return true;
}


