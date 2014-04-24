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

#include "trust_session_impl.h"
#include "mir/scene/session.h"
#include "mir/scene/trust_session_creation_parameters.h"
#include "mir/scene/trust_session_listener.h"
#include "session_container.h"

#include <sstream>
#include <algorithm>

namespace ms = mir::scene;

int next_unique_id = 0;

ms::TrustSessionImpl::TrustSessionImpl(
    std::weak_ptr<ms::Session> const& session,
    TrustSessionCreationParameters const&,
    std::shared_ptr<TrustSessionListener> const& trust_session_listener) :
    trusted_helper(session),
    trust_session_listener(trust_session_listener),
    state(mir_trust_session_state_stopped)
{
}

ms::TrustSessionImpl::~TrustSessionImpl()
{
    TrustSessionImpl::stop();
}

MirTrustSessionState ms::TrustSessionImpl::get_state() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return state;
}

std::weak_ptr<ms::Session> ms::TrustSessionImpl::get_trusted_helper() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return trusted_helper;
}

void ms::TrustSessionImpl::start()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_started)
        return;

    state = mir_trust_session_state_started;

    auto helper = trusted_helper.lock();
    if (helper) {
        helper->begin_trust_session();
    }
}

void ms::TrustSessionImpl::stop()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    state = mir_trust_session_state_stopped;

    auto helper = trusted_helper.lock();
    if (helper) {
        helper->end_trust_session();
    }

    std::vector<std::shared_ptr<ms::Session>> children;
    {
        std::lock_guard<decltype(mutex_children)> child_lock(mutex_children);

        for (auto rit = trusted_children.rbegin(); rit != trusted_children.rend(); ++rit)
        {
            auto session = (*rit).lock();
            if (session)
            {
                children.push_back(session);
            }
        }
        trusted_children.clear();
    }

    for (auto session : children)
    {
        trust_session_listener->trusted_session_ending(*this, session);
    }
}

bool ms::TrustSessionImpl::add_trusted_child(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return false;

    {
        std::lock_guard<decltype(mutex_children)> child_lock(mutex_children);

        if (std::find_if(trusted_children.begin(), trusted_children.end(),
                [session](std::weak_ptr<ms::Session> const& child)
                {
                    return child.lock() == session;
                }) != trusted_children.end())
        {
            return false;
        }

        trusted_children.push_back(session);
    }

    trust_session_listener->trusted_session_beginning(*this, session);
    return true;
}

void ms::TrustSessionImpl::remove_trusted_child(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    bool found = false;
    {
        std::lock_guard<decltype(mutex_children)> child_lock(mutex_children);

        for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
        {
            auto trusted_child = (*it).lock();
            if (trusted_child && trusted_child == session) {
                found = true;
                trusted_children.erase(it);
                break;
            }
        }
    }

    if (found)
    {
        trust_session_listener->trusted_session_ending(*this, session);
    }
}

void ms::TrustSessionImpl::for_each_trusted_child(
    std::function<void(std::shared_ptr<ms::Session> const&)> f,
    bool reverse) const
{
    std::lock_guard<decltype(mutex_children)> child_lock(mutex_children);

    if (reverse)
    {
        for (auto rit = trusted_children.rbegin(); rit != trusted_children.rend(); ++rit)
        {
            if (auto trusted_child = (*rit).lock())
                f(trusted_child);
        }
    }
    else
    {
        for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
        {
            if (auto trusted_child = (*it).lock())
                f(trusted_child);
        }
    }
}
