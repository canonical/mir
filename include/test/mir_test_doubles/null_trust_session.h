/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_TRUST_SESSION_H_
#define MIR_TEST_DOUBLES_NULL_TRUST_SESSION_H_

#include "mir/scene/trust_session.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullTrustSession : public scene::TrustSession
{
    MirTrustSessionState get_state() const override
    {
      return mir_trust_session_state_stopped;
    }

    void start() override
    {
    }

    void stop() override
    {
    }

    std::weak_ptr<scene::Session> get_trusted_helper() const override
    {
      return std::weak_ptr<scene::Session>();
    }

    bool add_trusted_child(std::shared_ptr<scene::Session> const&) override
    {
        return false;
    }

    bool remove_trusted_child(std::shared_ptr<scene::Session> const&) override
    {
        return false;
    }

    void for_each_trusted_child(std::function<void(std::shared_ptr<scene::Session> const&)>) const override
    {
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_TRUST_SESSION_H_ */
