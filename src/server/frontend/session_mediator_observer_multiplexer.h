/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_SESSION_MEDIATOR_OBSERVER_MULTIPLEXER_H_
#define MIR_FRONTEND_SESSION_MEDIATOR_OBSERVER_MULTIPLEXER_H_

#include "mir/observer_multiplexer.h"
#include "mir/frontend/session_mediator_observer.h"

namespace mir
{
namespace frontend
{
class SessionMediatorObserverMultiplexer : public ObserverMultiplexer<SessionMediatorObserver>
{
public:
    SessionMediatorObserverMultiplexer(std::shared_ptr<Executor> const& default_executor);

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

    void session_error(std::string const& app_name, char const* method, std::string const& what) override;

private:
    std::shared_ptr<Executor> const executor;
};
}
}

#endif //MIR_FRONTEND_SESSION_MEDIATOR_OBSERVER_MULTIPLEXER_H_

