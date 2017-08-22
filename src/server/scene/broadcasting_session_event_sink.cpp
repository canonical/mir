/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "broadcasting_session_event_sink.h"

namespace ms = mir::scene;

void ms::BroadcastingSessionEventSink::handle_focus_change(
    std::shared_ptr<Session> const& session)
{
    for_each([&](SessionEventSink* observer)
        { observer->handle_focus_change(session); });
}

void ms::BroadcastingSessionEventSink::handle_no_focus()
{
    for_each([&](SessionEventSink* observer)
        { observer->handle_no_focus(); });
}

void ms::BroadcastingSessionEventSink::handle_session_stopping(
    std::shared_ptr<Session> const& session)
{
    for_each([&](SessionEventSink* observer)
        { observer->handle_session_stopping(session); });
}

void ms::BroadcastingSessionEventSink::add(SessionEventSink* handler)
{
    ThreadSafeList<SessionEventSink*>::add(handler);
}

void ms::BroadcastingSessionEventSink::remove(SessionEventSink* handler)
{
    ThreadSafeList<SessionEventSink*>::remove(handler);
}
