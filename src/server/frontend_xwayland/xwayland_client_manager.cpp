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

#include "xwayland_client_manager.h"
#include "xwayland_log.h"
#include "../../include/server/mir/frontend/null_event_sink.h"
#include "mir/shell/shell.h"
#include "mir/scene/session.h"
#include "mir/log.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/frontend/session_credentials.h"

#include <unistd.h>
#include <sys/stat.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;

mf::XWaylandClientManager::Session::Session(XWaylandClientManager* manager, pid_t client_pid)
    : manager{manager},
      client_pid{client_pid},
      _session{manager->shell->open_session(client_pid, Fd{Fd::invalid}, "", std::make_shared<NullEventSink>())}
{
}

mf::XWaylandClientManager::Session::~Session()
{
    manager->shell->close_session(_session);
    manager->drop_expired(client_pid);
}

auto mf::XWaylandClientManager::Session::session() const -> std::shared_ptr<scene::Session>
{
    return _session;
}

mf::XWaylandClientManager::XWaylandClientManager(std::shared_ptr<shell::Shell> const& shell,
                                                 std::shared_ptr<SessionAuthorizer> const& session_authorizer)
    : shell{shell},
      session_authorizer{session_authorizer}
{
}

mf::XWaylandClientManager::~XWaylandClientManager()
{
    // drop_expired() should have been called for every owner before destroying
    if (!sessions_by_pid.empty())
    {
        log_warning(
            "XWaylandClientManager destroyed with %zu sessions in the map; "
            "if any XWaylandClientManager::Sessions are still alive, a crash is likely when they are destroyed",
            sessions_by_pid.size());
    }
}

auto mf::XWaylandClientManager::session_for_client(pid_t client_pid) -> std::shared_ptr<Session>
{
    std::lock_guard lock{mutex};

    std::shared_ptr<Session> session;
    auto const iter = sessions_by_pid.find(client_pid);
    if (iter != sessions_by_pid.end())
    {
        session = iter->second.lock();
    }

    if (session)
    {
        if (verbose_xwayland_logging_enabled())
        {
            log_info("Returning existing XWayland session for PID %d (use count %lu)", client_pid, session.use_count());
        }
    }
    else
    {
        auto const proc = "/proc/" + std::to_string(client_pid);

        struct stat proc_stat;
        if (stat(proc.c_str(), &proc_stat) == -1)
        {
            log_debug("Failed to get uid & gid for PID %d using stat(%s, ...), falling back to get(uid,gid)", 
                      client_pid, proc.c_str());

            proc_stat.st_uid = getuid();
            proc_stat.st_gid = getgid();
        }

        if (!session_authorizer->connection_is_allowed({client_pid, proc_stat.st_uid, proc_stat.st_gid}))
        {
            log_error("X11 session not authorized for PID %d, rejecting!", client_pid);
            return nullptr;
        }
        session = std::make_shared<Session>(this, client_pid);
        sessions_by_pid[client_pid] = session;
        if (verbose_xwayland_logging_enabled())
        {
            log_info("Created new XWayland session for PID %d", client_pid);
        }
    }

    return session;
}

void mf::XWaylandClientManager::drop_expired(pid_t client_pid)
{
    std::unique_lock lock{mutex};

    auto const iter = sessions_by_pid.find(client_pid);

    // Various rare special cases could cause this to fail (such as a new session being created after the weak_ptr to
    // the old one expired but before destructor called this function). If the PID is not associated with an expired
    // session it's safe to assume nothing needs to be done.
    if (iter != sessions_by_pid.end() && !iter->second.lock())
    {
        sessions_by_pid.erase(iter);
        if (verbose_xwayland_logging_enabled())
        {
            log_info("Closed XWayland session for PID %d", client_pid);
        }
    }
    else if (verbose_xwayland_logging_enabled())
    {
        log_info("XWaylandClientManager::drop_expired() called with PID %d, which is not expired", client_pid);
    }
}
