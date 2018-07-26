/*
 * Copyright © 2012, 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#ifndef MIR_REPORT_NULL_SESSION_MEDIATOR_REPORT_H_
#define MIR_REPORT_NULL_SESSION_MEDIATOR_REPORT_H_

#include "mir/frontend/session_mediator_observer.h"

#include <string>

namespace mir
{
namespace report
{
namespace null
{

// Do-nothing implementation to satisfy dependencies
class SessionMediatorReport : public frontend::SessionMediatorObserver
{
    void session_connect_called(std::string const& app_name) override;

    void session_create_surface_called(std::string const& app_name) override;

    void session_submit_buffer_called(std::string const& app_name) override;

    void session_allocate_buffers_called(std::string const& app_name) override;

    void session_release_buffers_called(std::string const& app_name) override;

    void session_release_surface_called(std::string const& app_name) override;

    void session_disconnect_called(std::string const& app_name) override;

    void session_configure_surface_called(std::string const& app_name) override;

    void session_configure_surface_cursor_called(std::string const& app_name) override;

    void session_configure_display_called(std::string const& app_name) override;

    void session_set_base_display_configuration_called(std::string const& app_name) override;

    void session_preview_base_display_configuration_called(std::string const& app_name) override;

    void session_confirm_base_display_configuration_called(std::string const& app_name) override;

    void session_start_prompt_session_called(std::string const& app_name, pid_t application_process) override;

    void session_stop_prompt_session_called(std::string const& app_name) override;

    void session_create_buffer_stream_called(std::string const& app_name) override;

    void session_release_buffer_stream_called(std::string const& app_name) override;

    void session_error(
        std::string const& app_name,
        char const* method,
        std::string const& what) override;
};
}
}
}


#endif /* MIR_FRONTEND_SESSION_MEDIATOR_REPORT_H_ */
