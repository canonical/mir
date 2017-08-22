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

#ifndef MIR_SCENE_SESSION_EVENT_SINK_H_
#define MIR_SCENE_SESSION_EVENT_SINK_H_

#include <memory>

namespace mir
{
namespace scene
{
class Session;

class SessionEventSink
{
public:
    virtual ~SessionEventSink() = default;

    virtual void handle_focus_change(std::shared_ptr<Session> const& session) = 0;
    virtual void handle_no_focus() = 0;
    virtual void handle_session_stopping(std::shared_ptr<Session> const& session) = 0;

protected:
    SessionEventSink() = default;
    SessionEventSink(SessionEventSink const&) = delete;
    SessionEventSink& operator=(SessionEventSink const&) = delete;
};

}
}

#endif /* MIR_SCENE_SESSION_EVENT_SINK_H_ */
