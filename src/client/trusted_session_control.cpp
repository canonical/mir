/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Nick Dedekind <nick.dedekind@gmail.com>
 */

#include "trusted_session_control.h"

namespace mcl = mir::client;

mcl::TrustedSessionControl::TrustedSessionControl() :
    next_fn_id(0)
{
}

mcl::TrustedSessionControl::~TrustedSessionControl()
{
}

int mcl::TrustedSessionControl::add_trusted_session_event_handler(std::function<void(uint32_t, MirTrustedSessionState)> const& fn)
{
    std::unique_lock<std::mutex> lk(guard);

    int id = next_id();
    handle_trusted_session_events[id] = fn;
    return id;
}

void mcl::TrustedSessionControl::remove_trusted_session_event_handler(int id)
{
    std::unique_lock<std::mutex> lk(guard);

    handle_trusted_session_events.erase(id);
}

void mcl::TrustedSessionControl::call_trusted_session_event_handler(int32_t id, uint32_t state)
{
    std::unique_lock<std::mutex> lk(guard);

    for (auto const& fn : handle_trusted_session_events)
    {
        fn.second(id, static_cast<MirTrustedSessionState>(state));
    }
}

int mcl::TrustedSessionControl::next_id()
{
    return next_fn_id.fetch_add(1);
}