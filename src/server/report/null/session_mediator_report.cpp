/*
 * Copyright © 2012 Canonical Ltd.
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

void mir::report::null::SessionMediatorReport::session_connect_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_create_surface_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_next_buffer_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_exchange_buffer_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_submit_buffer_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_allocate_buffers_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_release_buffers_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_release_surface_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_disconnect_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_drm_auth_magic_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_configure_surface_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_configure_surface_cursor_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_configure_display_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_start_prompt_session_called(std::string const&, pid_t)
{
}

void mir::report::null::SessionMediatorReport::session_stop_prompt_session_called(std::string const&)
{
}

void mir::report::null::SessionMediatorReport::session_error(
        std::string const&,
        char const* ,
        std::string const& )
{
}
