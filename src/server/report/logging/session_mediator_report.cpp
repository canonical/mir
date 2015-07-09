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

#include "session_mediator_report.h"

#include "mir/logging/logger.h"

namespace
{
char const* const component = "frontend::SessionMediator";
}

namespace ml = mir::logging;
namespace mrl = mir::report::logging;

mrl::SessionMediatorReport::SessionMediatorReport(std::shared_ptr<ml::Logger> const& log) :
    log(log)
{
}

void mrl::SessionMediatorReport::session_connect_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_connect(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_create_surface_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_create_surface(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_next_buffer_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_next_buffer_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_exchange_buffer_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_exchange_buffer_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_submit_buffer_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_submit_buffer_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_allocate_buffers_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_allocate_buffers_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_release_buffers_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_release_buffers_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_release_surface_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_release_surface_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_disconnect_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_disconnect_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_drm_auth_magic_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_drm_auth_magic_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_configure_surface_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_configure_surface_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_configure_surface_cursor_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_configure_surface_cursor_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_configure_display_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_configure_display_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_start_prompt_session_called(std::string const& app_name, pid_t application_process)
{
    log->log(ml::Severity::informational, "session_start_prompt_session_called(\"" + app_name + ", " + std::to_string(application_process) + ")", component);
}

void mrl::SessionMediatorReport::session_stop_prompt_session_called(std::string const& app_name)
{
    log->log(ml::Severity::informational, "session_stop_prompt_session_called(\"" + app_name + "\")", component);
}

void mrl::SessionMediatorReport::session_error(
        std::string const& app_name,
        char const* method,
        std::string const& what)
{
    log->log(ml::Severity::error, std::string(method) + " - session_error(\"" + app_name + "\"):\n" + what, component);
}
