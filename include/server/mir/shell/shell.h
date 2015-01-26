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

#ifndef MIR_SHELL_SHELL_H_
#define MIR_SHELL_SHELL_H_

#include "mir/shell/focus_controller.h"
#include "mir/frontend/surface_id.h"

#include "mir_toolkit/common.h"

#include <memory>

namespace mir
{
namespace frontend { class EventSink; }
namespace scene
{
class PromptSession;
class PromptSessionManager;
class PromptSessionCreationParameters;
class SessionCoordinator;
class Surface;
class SurfaceCoordinator;
class SurfaceCreationParameters;
}

namespace shell
{
class InputTargeter;

class Shell :
// TODO public virtual graphics::DisplayConfigurationPolicy,
    public virtual FocusController
{
public:
/** @name these functions support frontend requests
 *  @{ */
    virtual std::shared_ptr<scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) = 0;

    virtual void close_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual void handle_surface_created(std::shared_ptr<scene::Session> const& session) = 0;

    virtual std::shared_ptr<scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) = 0;

    virtual void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) = 0;

    virtual void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session) = 0;

    virtual frontend::SurfaceId create_surface(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params) = 0;

    virtual void destroy_surface(std::shared_ptr<scene::Session> const& session, frontend::SurfaceId surface) = 0;

    virtual int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        frontend::SurfaceId surface_id,
        MirSurfaceAttrib attrib,
        int value) = 0;

    virtual int get_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        frontend::SurfaceId surface_id,
        MirSurfaceAttrib attrib) = 0;
/** @} */
};

/// A placeholder for stuff all shells will want
class AbstractShell : public Shell
{
public:
    AbstractShell(
        std::shared_ptr<InputTargeter> const& input_targeter,
        std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager);

    ~AbstractShell() noexcept;

protected:
    std::shared_ptr<InputTargeter> const input_targeter;
    std::shared_ptr<scene::SurfaceCoordinator> const surface_coordinator;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator;
    std::shared_ptr<scene::PromptSessionManager> const prompt_session_manager;
};
}
}

#endif /* MIR_SHELL_SHELL_H_ */
