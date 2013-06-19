/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_CONNECTED_SESSIONS_H_
#define MIR_FRONTEND_CONNECTED_SESSIONS_H_

#include <memory>
#include <mutex>
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
    ~ConnectedSessions() { clear(); }

    void add(std::shared_ptr<Session> const& session)
    {
        std::unique_lock<std::mutex> lock(mutex);
        shell_list[session->id()] = session;
    }

    void remove(int id)
    {
        std::unique_lock<std::mutex> lock(mutex);
        shell_list.erase(id);
    }

    bool includes(int id) const
    {
        std::unique_lock<std::mutex> lock(mutex);
        return shell_list.find(id) != shell_list.end();
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex);
        shell_list.clear();
    }


private:
    ConnectedSessions(ConnectedSessions const&) = delete;
    ConnectedSessions& operator =(ConnectedSessions const&) = delete;

    std::mutex mutex;
    std::map<int, std::shared_ptr<Session>> shell_list;
};
}
}
}

#endif // MIR_FRONTEND_CONNECTED_SESSIONS_H_
