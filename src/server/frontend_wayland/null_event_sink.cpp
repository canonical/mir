/*
 * Copyright © 2018 Canonical Ltd.
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

#include "null_event_sink.h"

namespace mf = mir::frontend;

void mf::NullEventSink::send_buffer(BufferStreamId /*id*/, graphics::Buffer& /*buffer*/, graphics::BufferIpcMsgType)
{
}

void mf::NullEventSink::handle_event(MirEvent const& e)
{
    switch(mir_event_get_type(&e))
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

void mf::NullEventSink::add_buffer(mir::graphics::Buffer&)
{
}

void mf::NullEventSink::error_buffer(mir::geometry::Size, MirPixelFormat, std::string const&)
{
}

void mf::NullEventSink::update_buffer(mir::graphics::Buffer&)
{
}