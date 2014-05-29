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
#include "mir/scene/trust_session_listener.h"

namespace ms = mir::scene;

ms::TrustSessionImpl::TrustSessionImpl(
    std::weak_ptr<Session> const& session,
    TrustSessionCreationParameters const&,
    std::shared_ptr<TrustSessionListener> const& trust_session_listener) :
    trusted_helper(session),
    trust_session_listener(trust_session_listener),
    state(mir_trust_session_state_started)
{
    if (auto helper = trusted_helper.lock())
    {
        helper->start_trust_session();
    }
}

ms::TrustSessionImpl::~TrustSessionImpl()
{
    stop();
}

std::weak_ptr<ms::Session> ms::TrustSessionImpl::get_trusted_helper() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return trusted_helper;
}

void ms::TrustSessionImpl::stop()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    state = mir_trust_session_state_stopped;

    if (auto helper = trusted_helper.lock())
        helper->stop_trust_session();
}
