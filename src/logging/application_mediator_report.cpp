/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/logging/application_mediator_report.h"

#include "mir/logging/logger.h"

namespace
{
char const* const component = "frontend";
}

namespace ml = mir::logging;


ml::ApplicationMediatorReport::ApplicationMediatorReport(std::shared_ptr<Logger> const& log) :
    log(log)
{
}

void ml::ApplicationMediatorReport::application_connect_called(std::string const& app_name)
{
    log->log<Logger::informational>("application_connect(\"" + app_name + "\")", component);
}

void ml::ApplicationMediatorReport::application_create_surface_called(std::string const& app_name)
{
    log->log<Logger::informational>("application_create_surface(\"" + app_name + "\")", component);
}

void ml::ApplicationMediatorReport::application_next_buffer_called(std::string const& app_name)
{
    log->log<Logger::informational>("application_next_buffer_called(\"" + app_name + "\")", component);
}

void ml::ApplicationMediatorReport::application_release_surface_called(std::string const& app_name)
{
    log->log<Logger::informational>("application_release_surface_called(\"" + app_name + "\")", component);
}

void ml::ApplicationMediatorReport::application_disconnect_called(std::string const& app_name)
{
    log->log<Logger::informational>("application_disconnect_called(\"" + app_name + "\")", component);
}

void ml::ApplicationMediatorReport::application_drm_auth_magic_called(std::string const& app_name)
{
    log->log<Logger::informational>("application_drm_auth_magic_called(\"" + app_name + "\")", component);
}

void ml::ApplicationMediatorReport::application_error(
        std::string const& app_name,
        char const* method,
        std::string const& what)
{
    log->log<Logger::error>(std::string(method) + " - application_error(\"" + app_name + "\"):\n" + what, component);
}
