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

#ifndef MIR_SCENE_BROADCASTING_SESSION_EVENT_SINK_H_
#define MIR_SCENE_BROADCASTING_SESSION_EVENT_SINK_H_

#include "mir/scene/session_event_sink.h"
#include "mir/scene/session_event_handler_register.h"

#include "mir/thread_safe_list.h"

namespace mir
{
namespace scene
{
class BroadcastingSessionEventSink : public SessionEventSink,
                                     public SessionEventHandlerRegister,
                                     private ThreadSafeList<SessionEventSink*>
{
public:
    void handle_focus_change(std::shared_ptr<Session> const& session) override;
    void handle_no_focus() override;
    void handle_session_stopping(std::shared_ptr<Session> const& session) override;

    void add(SessionEventSink* handler) override;
    void remove(SessionEventSink* handler) override;
};

}
}

#endif /* MIR_SCENE_BROADCASTING_SESSION_EVENT_SINK_H_ */
