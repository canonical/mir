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

#ifndef MIR_SHELL_BROADCASTING_SESSION_EVENT_SINK_H_
#define MIR_SHELL_BROADCASTING_SESSION_EVENT_SINK_H_

#include "session_event_sink.h"
#include "session_event_handler_register.h"

#include <vector>
#include <mutex>

namespace mir
{
namespace shell
{

class Session;

class BroadcastingSessionEventSink : public SessionEventSink,
                                     public SessionEventHandlerRegister
{
public:
    void handle_focus_change(std::shared_ptr<Session> const& session);
    void handle_no_focus();
    void handle_session_stopping(std::shared_ptr<Session> const& session);

    void register_focus_change_handler(
        std::function<void(std::shared_ptr<Session> const& session)> const& handler);
    void register_no_focus_handler(
        std::function<void()> const& handler);
    void register_session_stopping_handler(
        std::function<void(std::shared_ptr<Session> const& session)> const& handler);

private:
    std::mutex handler_mutex;
    std::vector<std::function<void(std::shared_ptr<Session> const&)>> focus_change_handlers;
    std::vector<std::function<void()>> no_focus_handlers;
    std::vector<std::function<void(std::shared_ptr<Session> const&)>> session_stopping_handlers;
};

}
}

#endif /* MIR_SHELL_BROADCASTING_SESSION_EVENT_SINK_H_ */
