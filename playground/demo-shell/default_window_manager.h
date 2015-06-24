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

#ifndef MIR_EXAMPLES_DEFAULT_WINDOW_MANAGER_H_
#define MIR_EXAMPLES_DEFAULT_WINDOW_MANAGER_H_

#include "mir/shell/window_manager.h"

namespace mir
{
namespace scene { class SessionCoordinator; }

namespace shell { class FocusController; class DisplayLayout; }

namespace examples
{
class DefaultWindowManager : public shell::WindowManager
{
public:
    explicit DefaultWindowManager(shell::FocusController* focus_controller,
        std::shared_ptr<shell::DisplayLayout> const& display_layout,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator);

    void add_session(std::shared_ptr<scene::Session> const& session) override;

    void remove_session(std::shared_ptr<scene::Session> const& session) override;

    frontend::SurfaceId add_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build) override;

    void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        shell::SurfaceSpecification const& modifications) override;

    void remove_surface(
        std::shared_ptr<scene::Session> const& session,
        std::weak_ptr<scene::Surface> const& surface) override;

    void add_display(geometry::Rectangle const& area) override;

    void remove_display(geometry::Rectangle const& area) override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    bool handle_touch_event(MirTouchEvent const* event) override;

    bool handle_pointer_event(MirPointerEvent const* event) override;

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override;

private:
    shell::FocusController* const focus_controller;
    std::shared_ptr<shell::DisplayLayout> const display_layout;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator;
};
}
}

#endif /* MIR_SHELL_DEFAULT_WINDOW_MANAGER_H_ */
