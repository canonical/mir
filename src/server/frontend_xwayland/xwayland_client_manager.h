/*
 * Copyright (C) Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_XWAYLAND_CLIENT_MANAGER_H
#define MIR_FRONTEND_XWAYLAND_CLIENT_MANAGER_H

#include <mir/fd.h>

#include <memory>
#include <mutex>
#include <unordered_map>

namespace mir
{
namespace shell
{
class Shell;
}
namespace scene
{
class Session;
}
namespace frontend
{
class SessionAuthorizer;

/// Keeps track of which session is associated with which XWayland client PID
class XWaylandClientManager
{
public:
    /// Creates, owns and destroyes a scene::Session
    /// Should not outlive the XWaylandClientManager that created it
    class Session
    {
    public:
        Session(XWaylandClientManager* manager, pid_t client_pid);
        ~Session();

        auto session() const -> std::shared_ptr<scene::Session>;

    private:
        XWaylandClientManager* const manager;
        pid_t const client_pid;
        std::shared_ptr<scene::Session> const _session;
    };

    XWaylandClientManager(std::shared_ptr<shell::Shell> const& shell,
                          std::shared_ptr<SessionAuthorizer> const& session_authorizer);
    ~XWaylandClientManager();

    auto session_for_client(pid_t client_pid) -> std::shared_ptr<Session>;

private:
    void drop_expired(pid_t client_pid);

    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<SessionAuthorizer> const session_authorizer;
    std::mutex mutex;
    std::unordered_map<pid_t, std::weak_ptr<Session>> sessions_by_pid;
};

}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIENT_MANAGER_H
