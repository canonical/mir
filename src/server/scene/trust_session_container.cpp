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

#include "trust_session_container.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

uint insertion_order = 0;

ms::TrustSessionContainer::TrustSessionContainer()
    : trust_session_index(process_map)
    , process_index(get<1>(process_map))
{
}

bool ms::TrustSessionContainer::insert(std::shared_ptr<mf::TrustSession> const& trust_session, ClientProcess const& process)
{
    std::unique_lock<std::mutex> lk(mutex);

    object_by_trust_session::iterator it;
    bool valid = false;

    Object obj{trust_session, process, insertion_order++};
    boost::tie(it,valid) = process_map.insert(obj);

    return valid;
}

void ms::TrustSessionContainer::for_each_process_for_trust_session(
    std::shared_ptr<mf::TrustSession> const& trust_session,
    std::function<void(ClientProcess const& id)> f)
{
    std::unique_lock<std::mutex> lk(mutex);

    object_by_trust_session::iterator it,end;
    boost::tie(it,end) = trust_session_index.equal_range(trust_session.get());

    for (; it != end; ++it)
    {
        Object const& obj = *it;
        f(obj.client_process);
    }
}

void ms::TrustSessionContainer::for_each_trust_session_for_process(
    ClientProcess const& process,
    std::function<void(std::shared_ptr<mf::TrustSession> const&)> f)
{
    std::unique_lock<std::mutex> lk(mutex);

    object_by_process::iterator it,end;
    boost::tie(it,end) = process_index.equal_range(process);

    for (; it != end; ++it)
    {
        Object const& obj = *it;
        f(obj.trust_session);
    }
}

void ms::TrustSessionContainer::remove_trust_session(frontend::TrustSession* trust_session)
{
    (void)trust_session;
    std::unique_lock<std::mutex> lk(mutex);

    object_by_trust_session::iterator it,end;
    boost::tie(it,end) = trust_session_index.equal_range(trust_session);

    trust_session_index.erase(it, end);
}

void ms::TrustSessionContainer::remove_process(ClientProcess const& process)
{
    std::unique_lock<std::mutex> lk(mutex);

    object_by_process::iterator it,end;
    boost::tie(it,end) = process_index.equal_range(process);

    process_index.erase(it, end);
}
