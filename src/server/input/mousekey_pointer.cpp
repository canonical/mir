/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/input/mousekey_pointer.h"
#include "mir/input/input_sink.h"


mir::input::MousekeyPointer::MousekeyPointer(
    std::shared_ptr<MainLoop> main_loop, std::shared_ptr<input::InputEventTransformer> iet) :
    mir::input::VirtualInputDevice{"mousekey-pointer", mir::input::DeviceCapability::pointer},
    main_loop{std::move(main_loop)},
    iet{std::move(iet)}
{
}

bool mir::input::MousekeyPointer::handle(MirEvent const& event)
{
    auto handled = false;
    if_started_then(
        [this, &event, &handled](auto* sink, auto* builder)
        {
            handled = iet->transform(
                event,
                builder,
                [this, sink](auto event) { main_loop->spawn([sink, event] { sink->handle_input(event); }); });
        });
    return handled;
}

