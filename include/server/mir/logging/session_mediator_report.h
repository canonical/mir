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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_LOGGING_SESSION_MEDIATOR_REPORT_H_
#define MIR_LOGGING_SESSION_MEDIATOR_REPORT_H_

#include "mir/frontend/session_mediator_report.h"

#include <memory>

namespace mir
{
namespace logging
{
class Logger;

class SessionMediatorReport : public frontend::SessionMediatorReport
{
public:
    SessionMediatorReport(std::shared_ptr<Logger> const& log);

    virtual void session_connect_called(std::string const& app_name);

    virtual void session_create_surface_called(std::string const& app_name);

    virtual void session_next_buffer_called(std::string const& app_name);

    virtual void session_release_surface_called(std::string const& app_name);

    virtual void session_disconnect_called(std::string const& app_name);

    virtual void session_drm_auth_magic_called(std::string const& app_name);

    virtual void session_configure_surface_called(std::string const& app_name);

    virtual void session_configure_display_called(std::string const& app_name);

    virtual void session_error(
        std::string const& app_name,
        char const* method,
        std::string const& what);

private:
    std::shared_ptr<Logger> const log;
};

}
}


#endif /* MIR_LOGGING_SESSION_MEDIATOR_REPORT_H_ */
