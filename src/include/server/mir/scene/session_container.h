/*
 * Copyright © 2012-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SCENE_SESSION_CONTAINER_H_
#define MIR_SCENE_SESSION_CONTAINER_H_

#include <functional>
#include <vector>
#include <memory>
#include <mutex>

namespace mir
{
namespace scene
{
class Session;

class SessionContainer
{
public:
    SessionContainer();
    ~SessionContainer();

    void insert_session(std::shared_ptr<Session> const& session);
    void remove_session(std::shared_ptr<Session> const& session);

    void for_each(std::function<void(std::shared_ptr<Session> const&)> f) const;

    // For convenience the successor of the null session is defined as the last session
    // which would be passed to the for_each callback
    auto successor_of(std::shared_ptr<Session> const&) const -> std::shared_ptr<Session> ;
    auto predecessor_of(std::shared_ptr<Session> const&) const -> std::shared_ptr<Session> ;

    SessionContainer(const SessionContainer&) = delete;
    SessionContainer& operator=(const SessionContainer&) = delete;

private:
    std::vector<std::shared_ptr<Session>> apps;
    mutable std::mutex guard;
};

}
}


#endif // MIR_SCENE_SESSION_CONTAINER_H_
