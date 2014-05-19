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

#ifndef MIR_SCENE_TRUST_SESSION_CONTAINER_H_
#define MIR_SCENE_TRUST_SESSION_CONTAINER_H_

#include <sys/types.h>
#include <mutex>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace mir
{
namespace frontend
{
class TrustSession;
}

using boost::multi_index_container;
using namespace boost::multi_index;

namespace scene
{

// TODO {arg} TrustSessionContainer is internal to scene and should work in
// TODO {arg} terms of scene::TrustSession
class TrustSessionContainer
{
public:
    TrustSessionContainer();
    virtual ~TrustSessionContainer() = default;

    // {arg} I'm unconvinced this typedef is useful. Certainly "ClientProcess const&" is a pointless ref
    typedef pid_t ClientProcess;

    bool insert(std::shared_ptr<frontend::TrustSession> const& trust_session, ClientProcess const& process);

    void for_each_process_for_trust_session(std::shared_ptr<frontend::TrustSession> const& trust_session, std::function<void(ClientProcess const&)> f);
    void for_each_trust_session_for_process(ClientProcess const& process, std::function<void(std::shared_ptr<frontend::TrustSession> const&)> f);

    void remove_trust_session(std::shared_ptr<frontend::TrustSession> const& trust_session);
    void remove_process(ClientProcess const& process);

private:
    std::mutex mutable mutex;

    typedef struct {
        std::shared_ptr<frontend::TrustSession> trust_session;
        ClientProcess client_process;
        uint insert_order;
    } Object;

    typedef multi_index_container<
        Object,
        indexed_by<
            ordered_non_unique<
                composite_key<
                    Object,
                    member<Object, std::shared_ptr<frontend::TrustSession>, &Object::trust_session>,
                    member<Object, uint, &Object::insert_order>
                >
            >,
            ordered_unique<
                composite_key<
                    Object,
                    member<Object, ClientProcess, &Object::client_process>,
                    member<Object, std::shared_ptr<frontend::TrustSession>, &Object::trust_session>
                >
            >
        >
    > TrustSessionsProcesses;

    typedef nth_index<TrustSessionsProcesses,0>::type object_by_trust_session;
    typedef nth_index<TrustSessionsProcesses,1>::type object_by_process;


    TrustSessionsProcesses process_map;
    object_by_trust_session& trust_session_index;
    object_by_process& process_index;
};

}
}

#endif // MIR_SCENE_TRUST_SESSION_CONTAINER_H_
