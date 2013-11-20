/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_SURFACES_SESSION_CONTAINER_H_
#define MIR_SURFACES_SESSION_CONTAINER_H_

#include <vector>
#include <memory>
#include <mutex>

namespace mir
{
namespace shell { class Session; }

namespace surfaces
{
class SessionContainer
{
public:
    virtual void insert_session(std::shared_ptr<shell::Session> const& session) = 0;
    virtual void remove_session(std::shared_ptr<shell::Session> const& session) = 0;

    virtual void for_each(std::function<void(std::shared_ptr<shell::Session> const&)> f) const = 0;

    // For convenience the successor of the null session is defined as the last session
    // which would be passed to the for_each callback
    virtual std::shared_ptr<shell::Session> successor_of(std::shared_ptr<shell::Session> const&) const = 0;

protected:
    SessionContainer() = default;
    virtual ~SessionContainer() = default;

    SessionContainer(const SessionContainer&) = delete;
    SessionContainer& operator=(const SessionContainer&) = delete;
};

}
}


#endif // MIR_SURFACES_SESSION_CONTAINER_H_
