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

#ifndef MIR_SHELL_TRUST_SESSION_LISTENER_H_
#define MIR_SHELL_TRUST_SESSION_LISTENER_H_

#include <memory>

namespace mir
{
namespace scene { class Session; }
namespace shell
{
class TrustSession;

class TrustSessionListener
{
public:
    virtual void starting(std::shared_ptr<TrustSession> const& trust_session) = 0;
    virtual void stopping(std::shared_ptr<TrustSession> const& trust_session) = 0;

    virtual void trusted_session_beginning(TrustSession& trust_session, std::shared_ptr<scene::Session> const& session) = 0;
    virtual void trusted_session_ending(TrustSession& trust_session, std::shared_ptr<scene::Session> const& session) = 0;

protected:
    TrustSessionListener() = default;
    virtual ~TrustSessionListener() = default;

    TrustSessionListener(const TrustSessionListener&) = delete;
    TrustSessionListener& operator=(const TrustSessionListener&) = delete;
};

}
}


#endif // MIR_SHELL_SESSION_LISTENER_H_
