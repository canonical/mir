/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_DEFAULT_SHELL_H_
#define MIR_SHELL_DEFAULT_SHELL_H_

#include "mir/frontend/shell.h"
#include "mir/shell/focus_controller.h"

#include <mutex>

namespace mir
{
namespace scene { class SessionCoordinator; class Surface; class SurfaceCoordinator; }

namespace shell
{
class InputTargeter;

class DefaultShell : public frontend::Shell, public FocusController
{
public:
    DefaultShell(
        std::shared_ptr<InputTargeter> const& input_targeter,
        std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator);

    void focus_next() override;

    std::weak_ptr<scene::Session> focussed_application() const override;

    void set_focus_to(std::shared_ptr<scene::Session> const& focus) override;

    virtual std::shared_ptr<frontend::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override;

    virtual void close_session(std::shared_ptr<frontend::Session> const& session) override;

    void handle_surface_created(std::shared_ptr<frontend::Session> const& session) override;

    std::shared_ptr<frontend::PromptSession> start_prompt_session_for(
        std::shared_ptr<frontend::Session> const& session,
        scene::PromptSessionCreationParameters const& params) override;

    void add_prompt_provider_for(
        std::shared_ptr<frontend::PromptSession> const& prompt_session,
        std::shared_ptr<frontend::Session> const& session) override;

    void stop_prompt_session(std::shared_ptr<frontend::PromptSession> const& prompt_session) override;

    frontend::SurfaceId create_surface(std::shared_ptr<frontend::Session> const& session, scene::SurfaceCreationParameters const& params) override;

    void destroy_surface(std::shared_ptr<frontend::Session> const& session, frontend::SurfaceId surface) override;

    int set_surface_attribute(
        std::shared_ptr<frontend::Session> const& session,
        frontend::SurfaceId surface_id,
        MirSurfaceAttrib attrib,
        int value) override;

    int get_surface_attribute(
        std::shared_ptr<frontend::Session> const& session,
        frontend::SurfaceId surface_id,
        MirSurfaceAttrib attrib) override;

private:
    std::shared_ptr<InputTargeter> const input_targeter;
    std::shared_ptr<scene::SurfaceCoordinator> const surface_coordinator;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator;

    std::mutex surface_focus_lock;
    std::weak_ptr<scene::Surface> currently_focused_surface;

    std::mutex mutex;
    std::weak_ptr<scene::Session> focus_application;

    void set_focus_to_locked(std::unique_lock<std::mutex> const& lock, std::shared_ptr<scene::Session> const& next_focus);
};
}
}

#endif /* MIR_SHELL_DEFAULT_SHELL_H_ */
