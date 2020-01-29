/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 *
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_FRONTEND_SESSION_AUTHORIZER_H_
#define MIR_FRONTEND_SESSION_AUTHORIZER_H_



namespace mir
{
namespace frontend
{

class SessionCredentials;

class SessionAuthorizer
{
public:
    virtual ~SessionAuthorizer() = default;

    virtual bool connection_is_allowed(SessionCredentials const& creds) = 0;
    virtual bool configure_display_is_allowed(SessionCredentials const& creds) = 0;
    virtual bool set_base_display_configuration_is_allowed(SessionCredentials const& creds) = 0;
    virtual bool screencast_is_allowed(SessionCredentials const& creds) = 0;
    virtual bool prompt_session_is_allowed(SessionCredentials const&  creds) = 0;
    virtual bool configure_input_is_allowed(SessionCredentials const& creds) = 0;
    virtual bool set_base_input_configuration_is_allowed(SessionCredentials const& creds) = 0;

protected:
    SessionAuthorizer() = default;
    SessionAuthorizer(SessionAuthorizer const&) = delete;
    SessionAuthorizer& operator=(SessionAuthorizer const&) = delete;
};

}

} // namespace mir

#endif // MIR_FRONTEND_SESSION_AUTHORIZER_H_
