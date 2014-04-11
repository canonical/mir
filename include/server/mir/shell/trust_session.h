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

#ifndef MIR_SHELL_TRUST_SESSION_H_
#define MIR_SHELL_TRUST_SESSION_H_

#include "mir/frontend/trust_session.h"

namespace mir
{

namespace shell
{
class Session;

class TrustSession : public frontend::TrustSession
{
public:
    virtual std::weak_ptr<shell::Session> get_trusted_helper() const = 0;

    virtual bool add_trusted_child(std::shared_ptr<shell::Session> const& session) = 0;
    virtual void remove_trusted_child(std::shared_ptr<shell::Session> const& session) = 0;
    virtual void for_each_trusted_child(std::function<void(std::shared_ptr<shell::Session> const&)> f, bool reverse) const = 0;
};

}
}

#endif // MIR_SHELL_TRUST_SESSION_H_
