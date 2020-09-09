/*
 * Copyright (C) 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_XWAYLAND_CLIENT_MANAGER_H
#define MIR_FRONTEND_XWAYLAND_CLIENT_MANAGER_H

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

class XWaylandClientManager
{
public:
    XWaylandClientManager(std::shared_ptr<shell::Shell> const& shell);
    ~XWaylandClientManager();

    /// Returns the session associated with the given client_pid, or creates a new one if needed
    /// Stores a strong reference to the session associated with the given owner_key
    auto register_owner_for_client(void* owner_key, pid_t client_pid) -> std::shared_ptr<scene::Session>;

    /// Unassociates the session from owner_key
    /// Closes the session if it is not associated with any other owners
    void release(void* owner_key);

private:
    struct SessionInfo
    {
        std::shared_ptr<scene::Session> session;
        /// We could use session->process_id() instead, but storing our own copy is cheap and prevents internal memory
        /// safety from depending on external implementations
        pid_t pid;
        int owner_count;
    };

    std::shared_ptr<shell::Shell> const shell;
    std::mutex mutex;
    std::unordered_map<pid_t, SessionInfo*> sessions_by_pid;
    std::unordered_map<void*, SessionInfo*> sessions_by_owner;
};

}
}

#endif // MIR_FRONTEND_XWAYLAND_CLIENT_MANAGER_H
