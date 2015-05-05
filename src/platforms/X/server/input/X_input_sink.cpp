/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "X_input_sink.h"
#include "../../debug.h"

namespace mi = mir::input;
namespace mix = mi::X;

void mix::XInputSink::handle_input(MirEvent& /* event */)
{
    CALLED
#if 0
    // we attach the device id here, since this instance the first being able to maintains the uniqueness of the ids..
    // TODO avoid the MIR_INCLUDE_DEPRECATED_EVENT_HEADER in some way
    if (event.type == mir_event_type_key)
    {
        event.key.device_id = device_id;
    }
    if (event.type == mir_event_type_motion)
    {
        event.motion.device_id = device_id;
    }
    auto type = mir_event_get_type(&event);

    if (type != mir_event_type_input)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input event receivd from device"));
    dispatcher->dispatch(event);
#endif
}
