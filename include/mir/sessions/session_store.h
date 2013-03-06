/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SESSIONS_SESSION_STORE_H_
#define MIR_SESSIONS_SESSION_STORE_H_

#include <memory>
#include "mir/shell/shell.h"

namespace mir
{

namespace sessions
{

class Session;

/*
 * The word "Session" is overloaded and over-abstracted here. "Session" can
 * be used to represent either an application or a login. Both are sessional
 * concepts capable of creating and destroying surfaces. This seems
 * straightforward but what then is a SessionStore?
 *
 * A SessionStore is anything capable of opening and closing sessions. So by
 * the previous definition, a SessionStore could be any of:
 *  - a single login (creates App sessions, but is also a session itself)
 *  - a single seat (usually one per machine; creates login sessions)
 *
 * It seems feasible that in future someone will specialize Session and
 * SessionStore into more concrete classes like "Application", "Login" and
 * "Seat". That would be easier to comprehend, but is a job for another day.
 * - Daniel
 */

class SessionStore
{
public:
    virtual ~SessionStore() {}

    virtual std::shared_ptr<Session> open_session(std::string const& name) = 0;
    virtual void close_session(std::shared_ptr<Session> const& session)  = 0;

    virtual void tag_session_with_lightdm_id(std::shared_ptr<Session> const& session, int id) = 0;
    virtual void focus_session_with_lightdm_id(int id) = 0;

    virtual void shutdown() = 0;

    virtual std::shared_ptr<mir::Shell> current_shell() const = 0;

protected:
    SessionStore() = default;
    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;
};

}
}

#endif // MIR_SESSIONS_SESSION_STORE_H_
