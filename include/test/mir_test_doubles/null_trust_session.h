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

#include "mir/shell/trust_session.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullTrustSession : public shell::TrustSession
{

    MirTrustSessionAddTrustResult add_trusted_client_process(pid_t)
    {
        return mir_trust_session_add_tust_failed;
    }

    void for_each_trusted_client_process(std::function<void(pid_t)>, bool) const
    {
    }

    MirTrustSessionState get_state() const override
    {
      return mir_trust_session_state_stopped;
    }

    std::string get_cookie() const override
    {
      return "";
    }

    void start() override
    {
    }

    void stop() override
    {
    }

    std::weak_ptr<shell::Session> get_trusted_helper() const override
    {
      return std::weak_ptr<shell::Session>();
    }

    bool add_trusted_child(std::shared_ptr<shell::Session> const&) override
    {
        return false;
    }

    void remove_trusted_child(std::shared_ptr<shell::Session> const&) override
    {
    }

    void for_each_trusted_child(std::function<void(std::shared_ptr<shell::Session> const&)>,
                                bool) const override
    {
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_TRUST_SESSION_H_ */
