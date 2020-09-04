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

#include "xwayland_client_manager.h"
#include "null_event_sink.h"
#include "mir/shell/shell.h"
#include "mir/scene/session.h"
#include "mir/log.h"

#include "boost/throw_exception.hpp"

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;

mf::XWaylandClientManager::XWaylandClientManager(std::shared_ptr<shell::Shell> const& shell)
    : shell{shell}
{
}

mf::XWaylandClientManager::~XWaylandClientManager()
{
    // release() should have been called for every owner before destroying
    if (!sessions_by_pid.empty() || !sessions_by_owner.empty())
    {
        log_warning(
            "XWaylandClientManager destroyed without all sessions being released "
            "(sessions_by_pid: %lu, sessions_by_owner: %lu)",
            sessions_by_pid.size(),
            sessions_by_owner.size());

        // Clean up to prevent memory leaks

        std::vector<void*> forgetful_owners;
        for (auto const& iter : sessions_by_owner)
        {
            forgetful_owners.push_back(iter.first);
        }

        for (auto const& owner : forgetful_owners)
        {
            release(owner);
        }
    }
}

auto mf::XWaylandClientManager::get_session_for_client(
    void* owner_key, pid_t client_pid) -> std::shared_ptr<ms::Session>
{
    std::lock_guard<std::mutex> lock{mutex};

    if (sessions_by_owner.find(owner_key) != sessions_by_owner.end())
    {
        // This would happen if an owner tries to get a new session without release()ing the previous one
        BOOST_THROW_EXCEPTION(std::logic_error(
            "XWaylandClientManager::get_session_for_client() called with already existing owner_key"));
    }

    SessionInfo* info;
    auto const iter = sessions_by_pid.find(client_pid);
    if (iter == sessions_by_pid.end())
    {
        // We don't currently have an active session for the given PID, create a new one
        // Ideally we wouldn't run open_session() under lock, but that complicates the logic and isn't required
        auto const session = shell->open_session(client_pid, "", std::make_shared<NullEventSink>());
        // Start the owner count at 0, it will get incremented below
        // The info struct will be deleted in release() when the owner count hits 0 again
        info = new SessionInfo{session, client_pid, 0};
        sessions_by_pid.insert(std::make_pair(client_pid, info));
    }
    else
    {
        // We already have an active session for the given PID, use it
        info = iter->second;
    }

    info->owner_count++; // Changes the owner count from 0 to 1 the first time
    sessions_by_owner.insert(std::make_pair(owner_key, info));
    return info->session;
}

void mf::XWaylandClientManager::release(void* owner_key)
{
    std::unique_lock<std::mutex> lock{mutex};

    auto const iter = sessions_by_owner.find(owner_key);
    if (iter == sessions_by_owner.end())
    {
        // You must already be a session owner in order to call release()
        BOOST_THROW_EXCEPTION(std::logic_error(
            "XWaylandClientManager::release() called with unknown owner_key"));
    }

    auto const info = iter->second;
    sessions_by_owner.erase(iter);

    std::shared_ptr<scene::Session> session_to_close;
    info->owner_count--; // The session now has one less owner
    if (info->owner_count <= 0)
    {
        // When a session has no owners, close it and delete the info struct
        session_to_close = std::move(info->session);
        sessions_by_pid.erase(info->pid);
        delete info;
    }

    // Don't run close_session() under lock because there's no reason to
    lock.unlock();
    if (session_to_close)
    {
        shell->close_session(session_to_close);
    }
}
