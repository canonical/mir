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

#include "broadcasting_session_event_sink.h"

namespace ms = mir::surfaces;
namespace msh = mir::shell;

/*
 * TODO: Use Boost.Signals2 for this when we default to Boost 1.54, see
 * https://svn.boost.org/trac/boost/ticket/8102 . For now, just use a custom
 * mechanism with coarse locking. When emitting events, we copy the handlers
 * to a temporary vector to avoid calling handlers while locked, while still
 * being thread-safe.
 */

void ms::BroadcastingSessionEventSink::handle_focus_change(
    std::shared_ptr<msh::Session> const& session)
{
    std::vector<std::function<void(std::shared_ptr<msh::Session> const&)>> handlers;

    {
        std::lock_guard<std::mutex> lg{handler_mutex};
        handlers = focus_change_handlers;
    }

    for (auto& handler : handlers)
        handler(session);
}

void ms::BroadcastingSessionEventSink::handle_no_focus()
{
    std::vector<std::function<void()>> handlers;

    {
        std::lock_guard<std::mutex> lg{handler_mutex};
        handlers = no_focus_handlers;
    }

    for (auto& handler : handlers)
        handler();
}

void ms::BroadcastingSessionEventSink::handle_session_stopping(
    std::shared_ptr<msh::Session> const& session)
{
    std::vector<std::function<void(std::shared_ptr<msh::Session> const&)>> handlers;

    {
        std::lock_guard<std::mutex> lg{handler_mutex};
        handlers = session_stopping_handlers;
    }

    for (auto& handler : handlers)
        handler(session);
}

void ms::BroadcastingSessionEventSink::register_focus_change_handler(
    std::function<void(std::shared_ptr<msh::Session> const& session)> const& handler)
{
    std::lock_guard<std::mutex> lg{handler_mutex};

    focus_change_handlers.push_back(handler);
}

void ms::BroadcastingSessionEventSink::register_no_focus_handler(
    std::function<void()> const& handler)
{
    std::lock_guard<std::mutex> lg{handler_mutex};

    no_focus_handlers.push_back(handler);
}

void ms::BroadcastingSessionEventSink::register_session_stopping_handler(
    std::function<void(std::shared_ptr<msh::Session> const& session)> const& handler)
{
    std::lock_guard<std::mutex> lg{handler_mutex};

    session_stopping_handlers.push_back(handler);
}
