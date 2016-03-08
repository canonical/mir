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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "global_event_sender.h"
#include "session_container.h"
#include "mir/scene/session.h"

namespace mg=mir::graphics;
namespace ms=mir::scene;
namespace mi=mir::input;

ms::GlobalEventSender::GlobalEventSender(std::shared_ptr<SessionContainer> const& session_container)
    : sessions(session_container)
{
}

void ms::GlobalEventSender::handle_event(MirEvent const&)
{
    //TODO, no driving test cases, although messages like 'server shutdown' could go here
}

void ms::GlobalEventSender::handle_lifecycle_event(MirLifecycleState)
{
    // Lifecycle events are per application session, never global
}

void ms::GlobalEventSender::handle_display_config_change(mg::DisplayConfiguration const& config)
{
    sessions->for_each([&config](std::shared_ptr<ms::Session> const& session)
    {
        session->send_display_config(config);
    });
}

void ms::GlobalEventSender::handle_input_device_change(std::vector<std::shared_ptr<mi::Device>> const& devices)
{
    sessions->for_each([&devices](std::shared_ptr<ms::Session> const& session)
    {
        session->send_input_device_change(devices);
    });
}

void ms::GlobalEventSender::send_ping(int32_t)
{
    // Ping events are per-application session.
}

void ms::GlobalEventSender::send_buffer(mir::frontend::BufferStreamId, mg::Buffer&, mg::BufferIpcMsgType)
{
}

void ms::GlobalEventSender::send_buffer(mg::Buffer&, mg::BufferIpcMsgType)
{
}
