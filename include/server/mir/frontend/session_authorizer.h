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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_FRONTEND_SESSION_AUTHORIZER_H_
#define MIR_FRONTEND_SESSION_AUTHORIZER_H_

#include <sys/types.h>

namespace mir
{
namespace frontend
{

class SessionAuthorizer
{
public:
    virtual ~SessionAuthorizer() {}

    virtual bool connection_is_allowed(pid_t pid) = 0;
    virtual bool configure_display_is_allowed(pid_t pid) = 0;
protected:
    SessionAuthorizer() = default;
    SessionAuthorizer(SessionAuthorizer const&) = delete;
    SessionAuthorizer& operator=(SessionAuthorizer const&) = delete;
};

}

} // namespace mir

#endif // MIR_FRONTEND_SESSION_AUTHORIZER_H_
