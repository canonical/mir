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

#ifndef MIR_SCENE_NULL_TRUST_SESSION_LISTENER_H_
#define MIR_SCENE_NULL_TRUST_SESSION_LISTENER_H_

#include "mir/scene/trust_session_listener.h"

namespace mir
{
namespace scene
{

class NullTrustSessionListener : public TrustSessionListener
{
public:
    NullTrustSessionListener() = default;
    virtual ~NullTrustSessionListener() noexcept(true) = default;

    void starting(std::shared_ptr<TrustSession> const&) override {};
    void stopping(std::shared_ptr<TrustSession> const&) override {};

    void trusted_session_beginning(TrustSession const&, std::shared_ptr<Session> const&) override {};
    void trusted_session_ending(TrustSession const&, std::shared_ptr<Session> const&) override {};

protected:
    NullTrustSessionListener(const NullTrustSessionListener&) = delete;
    NullTrustSessionListener& operator=(const NullTrustSessionListener&) = delete;
};

}
}


#endif // MIR_SHELL_NULL_TRUST_SESSION_LISTENER_H_
