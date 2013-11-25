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

#ifndef MIR_SHELL_SESSION_LISTENER_H_
#define MIR_SHELL_SESSION_LISTENER_H_

#include <memory>

namespace mir
{
namespace shell
{
class Session;
class Surface;

class SessionListener
{
public:
    virtual void starting(std::shared_ptr<Session> const& session) = 0;
    virtual void stopping(std::shared_ptr<Session> const& session) = 0;
    virtual void focused(std::shared_ptr<Session> const& session) = 0;
    virtual void unfocused() = 0;

    virtual void surface_created(Session& session, std::shared_ptr<Surface> const& surface) = 0;
    virtual void destroying_surface(Session& session, std::shared_ptr<Surface> const& surface) = 0;

protected:
    SessionListener() = default;
    virtual ~SessionListener() = default;

    SessionListener(const SessionListener&) = delete;
    SessionListener& operator=(const SessionListener&) = delete;
};

}
}


#endif // MIR_SHELL_SESSION_LISTENER_H_
