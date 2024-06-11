/*
 * Copyright © Canonical Ltd.
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
 */

#include "null_event_sink.h"

namespace mf = mir::frontend;

void mf::NullEventSink::handle_event(EventUPtr&& event)
{
    switch(mir_event_get_type(event.get()))
    {
    default:
        // Do nothing
        break;
    }
}

void mf::NullEventSink::handle_lifecycle_event(MirLifecycleState /*state*/)
{
}

void mf::NullEventSink::handle_display_config_change(graphics::DisplayConfiguration const& /*config*/)
{
}

void mf::NullEventSink::send_ping(int32_t)
{
}

void mf::NullEventSink::handle_input_config_change(MirInputConfig const&)
{
}

void mf::NullEventSink::handle_error(mir::ClientVisibleError const&)
{
}
