/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "trust_session.h"
#include "mir/shell/trust_session_creation_parameters.h"
#include "mir/shell/session.h"
#include "session_container.h"

#include <sstream>
#include <algorithm>

namespace ms = mir::scene;
namespace msh = mir::shell;

struct Cookie
{
    int id;
    void* ptr;
};

namespace std
{
    template<>
    struct hash<Cookie>
    {
        typedef Cookie argument_type;
        typedef std::size_t value_type;

        value_type operator()(argument_type const& s) const
        {
            value_type const h1 ( std::hash<int>()(s.id) );
            value_type const h2 ( std::hash<void*>()(s.ptr) );
            return h1 ^ (h2 << 1);
        }
    };
}

int next_unique_id = 0;

ms::TrustSession::TrustSession(
    std::weak_ptr<msh::Session> const& session,
    msh::TrustSessionCreationParameters const&) :
    trusted_helper(session),
    state(mir_trust_session_state_stopped)
{
    Cookie id{next_unique_id++, this};
    std::hash<Cookie> hash_fn;

    std::stringstream ss;
    ss << hash_fn(id);
    cookie = ss.str();
}

ms::TrustSession::~TrustSession()
{
    TrustSession::stop();
}

MirTrustSessionState ms::TrustSession::get_state() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return state;
}

std::string ms::TrustSession::get_cookie() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return cookie;
}

std::weak_ptr<msh::Session> ms::TrustSession::get_trusted_helper() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return trusted_helper;
}

void ms::TrustSession::start()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    state = mir_trust_session_state_started;
}

void ms::TrustSession::stop()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    state = mir_trust_session_state_stopped;

    clear_trusted_children();
}

MirTrustSessionAddTrustResult ms::TrustSession::add_trusted_client_process(pid_t pid)
{
    std::lock_guard<decltype(mutex_children)> lock(mutex_processes);

    auto it = std::find(client_processes.begin(), client_processes.end(), pid);
    if (it != client_processes.end())
        return mir_trust_session_add_tust_duplicate;

    client_processes.push_back(pid);
    return mir_trust_session_add_tust_succeeded;
}

void ms::TrustSession::for_each_trusted_client_process(std::function<void(pid_t pid)> f, bool reverse) const
{
    std::lock_guard<decltype(mutex_children)> lock(mutex_processes);

    if (reverse)
    {
        for (auto rit = client_processes.rbegin(); rit != client_processes.rend(); ++rit)
        {
            f(*rit);
        }
    }
    else
    {
        for (auto it = client_processes.begin(); it != client_processes.end(); ++it)
        {
            f(*it);
        }
    }
}

bool ms::TrustSession::add_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    std::lock_guard<decltype(mutex_children)> lock(mutex_children);

    if (get_state() == mir_trust_session_state_stopped)
        return false;

    if (std::find_if(trusted_children.begin(), trusted_children.end(),
            [session](std::weak_ptr<shell::Session> const& child)
            {
                return child.lock() == session;
            }) != trusted_children.end())
    {
        return false;
    }

    trusted_children.push_back(session);
    return true;
}

void ms::TrustSession::remove_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    std::lock_guard<decltype(mutex_children)> lock(mutex_children);

    if (state == mir_trust_session_state_stopped)
        return;

    for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
    {
        auto trusted_child = (*it).lock();
        if (trusted_child == session) {

            trusted_children.erase(it);
            break;
        }
    }
}

void ms::TrustSession::for_each_trusted_child(
    std::function<void(std::shared_ptr<msh::Session> const&)> f,
    bool reverse) const
{
    std::lock_guard<decltype(mutex_children)> lock(mutex_children);

    if (reverse)
    {
        for (auto rit = trusted_children.rbegin(); rit != trusted_children.rend(); ++rit)
        {
            auto trusted_child = (*rit).lock();
            if (trusted_child)
                f(trusted_child);
        }
    }
    else
    {
        for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
        {
            auto trusted_child = (*it).lock();
            if (trusted_child)
                f(trusted_child);
        }
    }
}

void ms::TrustSession::clear_trusted_children()
{
    std::lock_guard<decltype(mutex_children)> lock(mutex_children);
    trusted_children.clear();
}
