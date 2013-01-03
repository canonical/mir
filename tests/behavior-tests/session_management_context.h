/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SESSION_MANAGEMENT_CONTEXT_H_
#define MIR_SESSION_MANAGER_CONTEXT_H_

#include <string>
#include <map>
#include <memory>

namespace mir
{

namespace frontend
{
class Session;
class SessionManager;
}

namespace test
{
namespace cucumber
{

class SessionManagementContext
{
public:
    SessionManagementContext();
    virtual ~SessionManagementContext();
    
    bool open_session(const std::string& session_name);

protected:
    SessionManagementContext(const SessionManagementContext&) = delete;
    SessionManagementContext& operator=(const SessionManagementContext&) = delete;
private:
    std::map<std::string, std::weak_ptr<frontend::Session>> open_sessions;
    std::shared_ptr<frontend::SessionManager> session_manager;
};

}
}
}

#endif // MIR_SESSION_MANAGEMENT_CONTEXT_H_
