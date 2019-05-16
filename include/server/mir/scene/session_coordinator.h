/*
 * Copyright © 2014-2019 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_SESSION_COORDINATOR_H_
#define MIR_SCENE_SESSION_COORDINATOR_H_

#include "mir/frontend/surface_id.h"

#include "mir_toolkit/common.h"

#include <memory>

namespace mir
{
namespace frontend
{
class EventSink;
}

namespace scene
{
class Session;
class SessionListener;
class Surface;
struct SurfaceCreationParameters;

class SessionCoordinator
{
public:
    virtual void set_focus_to(std::shared_ptr<Session> const& focus) = 0;
    virtual void unset_focus() = 0;

    virtual auto open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) -> std::shared_ptr<Session> = 0;

    virtual void close_session(std::shared_ptr<Session> const& session)  = 0;

    virtual auto successor_of(std::shared_ptr<Session> const&) const -> std::shared_ptr<Session> = 0;
    virtual auto predecessor_of(std::shared_ptr<Session> const&) const -> std::shared_ptr<Session> = 0;

    virtual void add_listener(std::shared_ptr<SessionListener> const&) = 0;
    virtual void remove_listener(std::shared_ptr<SessionListener> const&) = 0;
protected:
    SessionCoordinator() = default;
    virtual ~SessionCoordinator() = default;
    SessionCoordinator(SessionCoordinator const&) = delete;
    SessionCoordinator& operator=(SessionCoordinator const&) = delete;
};

}
}

#endif /* MIR_SCENE_SESSION_COORDINATOR_H_ */
