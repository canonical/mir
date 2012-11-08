/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_CONNECTED_SESSIONS_H_
#define MIR_FRONTEND_CONNECTED_SESSIONS_H_

#include <memory>
#include <map>

namespace mir
{
namespace frontend
{
namespace detail
{
template<class Session>
class ConnectedSessions
{
public:
    ConnectedSessions() {}
    ~ConnectedSessions() {}

    void add(std::shared_ptr<Session> const& session)
    {
        sessions_list[session->id()] = session;
    }

    void remove(int id)
    {
        sessions_list.erase(id);
    }

    bool includes(int id) const
    {
        return sessions_list.find(id) != sessions_list.end();
    }

    void clear()
    {
        sessions_list.clear();
    }


private:
    ConnectedSessions(ConnectedSessions const&) = delete;
    ConnectedSessions& operator =(ConnectedSessions const&) = delete;

    std::map<int, std::shared_ptr<Session>> sessions_list;
};
}
}
}

#endif // MIR_FRONTEND_CONNECTED_SESSIONS_H_
