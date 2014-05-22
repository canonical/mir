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

#ifndef MIR_SCENE_TRUST_SESSION_H_
#define MIR_SCENE_TRUST_SESSION_H_

#include "mir/frontend/trust_session.h"

namespace mir
{
namespace scene
{
class Session;

class TrustSession : public frontend::TrustSession
{
public:
    virtual std::weak_ptr<Session> get_trusted_helper() const = 0;

    virtual bool add_trusted_participant(std::shared_ptr<Session> const& session) = 0;
    virtual bool remove_trusted_participant(std::shared_ptr<Session> const& session) = 0;
};

}
}

#endif // MIR_SHELL_TRUST_SESSION_H_
