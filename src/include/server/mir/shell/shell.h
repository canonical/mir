/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SHELL_SHELL_H_
#define MIR_SHELL_SHELL_H_

#include "mir/shell/focus_controller.h"
#include "mir/input/event_filter.h"
#include "mir/compositor/display_listener.h"
#include "mir/fd.h"

#include "mir_toolkit/common.h"

#include <unistd.h>

#include <memory>

namespace mir
{
class Executor;
namespace wayland
{
template<typename>
class Weak;
}
namespace frontend
{
class WlSurface;
class EventSink;
}
namespace scene
{
class PromptSession;
class PromptSessionManager;
class PromptSessionCreationParameters;
class SessionCoordinator;
class Surface;
class SurfaceObserver;
}

namespace shell
{
class InputTargeter;
struct SurfaceSpecification;
class SurfaceStack;

class Shell :
    public virtual FocusController,
    public virtual input::EventFilter,
    public virtual compositor::DisplayListener
{
public:
/** @name these functions support frontend requests
 *  @{ */
    virtual auto open_session(
        pid_t client_pid,
        Fd socket_fd,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) -> std::shared_ptr<scene::Session> = 0;

    virtual void close_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual std::shared_ptr<scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) = 0;

    virtual void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) = 0;

    virtual void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session) = 0;

    virtual auto create_surface(
        std::shared_ptr<scene::Session> const& session,
        wayland::Weak<frontend::WlSurface> const& wayland_surface,
        SurfaceSpecification const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer,
        Executor* observer_executor) -> std::shared_ptr<scene::Surface> = 0;

    virtual void surface_ready(std::shared_ptr<scene::Surface> const& surface) = 0;

    virtual void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        shell::SurfaceSpecification  const& modifications) = 0;

    virtual void destroy_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface) = 0;

    virtual void raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) = 0;

    virtual void request_move(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirInputEvent const* event) = 0;

    virtual void request_resize(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirInputEvent const* event,
        MirResizeEdge edge) = 0;

/** @} */
};
}
}

#endif /* MIR_SHELL_SHELL_H_ */
