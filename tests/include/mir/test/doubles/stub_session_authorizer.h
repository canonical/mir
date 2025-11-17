/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_STUB_SESSION_AUTHORIZER_H_
#define MIR_TEST_DOUBLES_STUB_SESSION_AUTHORIZER_H_

#include "mir/frontend/session_authorizer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubSessionAuthorizer : public frontend::SessionAuthorizer
{
    bool connection_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
    bool configure_display_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
    bool set_base_display_configuration_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
    bool screencast_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
    bool prompt_session_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
    bool configure_input_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
    bool set_base_input_configuration_is_allowed(mir::frontend::SessionCredentials const&) override
    {
        return true;
    }
};

struct MockSessionAuthorizer : StubSessionAuthorizer
{
    MOCK_METHOD(bool, screencast_is_allowed, (frontend::SessionCredentials const&), ());
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SESSION_AUTHORIZER_H_
