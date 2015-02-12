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

#ifndef MIR_EXAMPLE_GENERIC_SHELL_H_
#define MIR_EXAMPLE_GENERIC_SHELL_H_

#include "server_example_shell.h"

#include "mir/scene/session.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/abstract_shell.h"

///\example server_example_generic_shell.h
/// A generic shell that supports a window manager

namespace mir
{
namespace examples
{
class WindowManager
{
public:
    virtual void add_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual void remove_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual frontend::SurfaceId add_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build) = 0;

    virtual void remove_surface(
        std::weak_ptr<scene::Surface> const& surface,
        std::shared_ptr<scene::Session> const& session) = 0;

    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;

    virtual bool handle_key_event(MirKeyInputEvent const* event) = 0;

    virtual bool handle_touch_event(MirTouchInputEvent const* event) = 0;

    virtual bool handle_pointer_event(MirPointerInputEvent const* event) = 0;

    virtual int handle_set_state(std::shared_ptr<scene::Surface> const& surface, MirSurfaceState value) = 0;

    virtual ~WindowManager() = default;
    WindowManager() = default;
    WindowManager(WindowManager const&) = delete;
    WindowManager& operator=(WindowManager const&) = delete;
};

class GenericShell : public virtual Shell,
    private shell::AbstractShell
{
public:
    GenericShell(
        std::shared_ptr<shell::InputTargeter> const& input_targeter,
        std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager);

    std::shared_ptr<scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override;

    void close_session(std::shared_ptr<scene::Session> const& session) override;

    frontend::SurfaceId create_surface(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params) override;

    void destroy_surface(std::shared_ptr<scene::Session> const& session, frontend::SurfaceId surface) override;

    bool handle(MirEvent const& event) override;

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override;

protected:
    // Ugly hack to ensure GenericShell is created before BasicShell creates the
    // window manager (otherwise the shell::FocusController interface isn't initialised)
    void init_window_manager(std::shared_ptr<WindowManager> const& new_window_manager)
    {
        window_manager = new_window_manager;
    }

private:
    void add_display(geometry::Rectangle const& area) override;
    void remove_display(geometry::Rectangle const& area) override;

    std::shared_ptr<WindowManager> window_manager;
};
}
}

#endif /* MIR_EXAMPLE_GENERIC_SHELL_H_ */
