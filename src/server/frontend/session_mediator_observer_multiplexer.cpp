/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "session_mediator_observer_multiplexer.h"

namespace mf = mir::frontend;

void mf::SessionMediatorObserverMultiplexer::session_connect_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_connect_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_create_surface_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_create_surface_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_exchange_buffer_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_exchange_buffer_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_submit_buffer_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_submit_buffer_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_allocate_buffers_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_allocate_buffers_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_release_buffers_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_release_buffers_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_release_surface_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_release_surface_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_disconnect_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_disconnect_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_configure_surface_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_configure_surface_called(app_name); });
}

void
mf::SessionMediatorObserverMultiplexer::session_configure_surface_cursor_called(std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_configure_surface_cursor_called(app_name);
        });
}

void mf::SessionMediatorObserverMultiplexer::session_configure_display_called(std::string const& app_name)
{
    for_each_observer([&app_name](auto& observer) { observer.session_configure_display_called(app_name); });
}

void mf::SessionMediatorObserverMultiplexer::session_set_base_display_configuration_called(
    std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_set_base_display_configuration_called(app_name);
        });
}

void mf::SessionMediatorObserverMultiplexer::session_preview_base_display_configuration_called(
    std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_preview_base_display_configuration_called(app_name);
        });
}

void mf::SessionMediatorObserverMultiplexer::session_confirm_base_display_configuration_called(
    std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_confirm_base_display_configuration_called(app_name);
        });
}

void mf::SessionMediatorObserverMultiplexer::session_start_prompt_session_called(
    std::string const& app_name,
    pid_t application_process)
{
    for_each_observer(
        [&app_name, application_process](auto& observer)
        {
            observer.session_start_prompt_session_called(app_name, application_process);
        });
}

void mf::SessionMediatorObserverMultiplexer::session_stop_prompt_session_called(
    std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_stop_prompt_session_called(app_name);
        });
}

void mf::SessionMediatorObserverMultiplexer::session_create_buffer_stream_called(
    std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_create_buffer_stream_called(app_name);
        });
}

void
mf::SessionMediatorObserverMultiplexer::session_release_buffer_stream_called(
    std::string const& app_name)
{
    for_each_observer(
        [&app_name](auto& observer)
        {
            observer.session_release_buffer_stream_called(app_name);
        });
}

void mir::frontend::SessionMediatorObserverMultiplexer::session_error(
    std::string const& app_name,
    char const* method,
    std::string const& what)
{
    for_each_observer(
        [&app_name, method, &what](auto& observer)
        {
            observer.session_error(app_name, method, what);
        });
}
