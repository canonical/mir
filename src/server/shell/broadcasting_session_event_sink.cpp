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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/shell/broadcasting_session_event_sink.h"

namespace msh = mir::shell;

void msh::BroadcastingSessionEventSink::handle_focus_change(
    std::shared_ptr<Session> const& session)
{
    focus_change_signal(session);
}

void msh::BroadcastingSessionEventSink::handle_no_focus()
{
    no_focus_signal();
}

void msh::BroadcastingSessionEventSink::handle_session_stopping(
    std::shared_ptr<Session> const& session)
{
    session_stopping_signal(session);
}

void msh::BroadcastingSessionEventSink::register_focus_change_handler(
    std::function<void(std::shared_ptr<Session> const& session)> const& handler)
{
    focus_change_signal.connect(handler);
}

void msh::BroadcastingSessionEventSink::register_no_focus_handler(
    std::function<void()> const& handler)
{
    no_focus_signal.connect(handler);
}

void msh::BroadcastingSessionEventSink::register_session_stopping_handler(
    std::function<void(std::shared_ptr<Session> const& session)> const& handler)
{
    session_stopping_signal.connect(handler);
}
