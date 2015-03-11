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

#ifndef MIR_SHELL_NULL_WINDOW_MANAGER_H_
#define MIR_SHELL_NULL_WINDOW_MANAGER_H_

#include "mir/shell/window_manager.h"

namespace mir
{
namespace shell
{
class NullWindowManager : public WindowManager
{
public:
    void add_session(std::shared_ptr<scene::Session> const& session) override;

    void remove_session(std::shared_ptr<scene::Session> const& session) override;

    frontend::SurfaceId add_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<frontend::SurfaceId(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params)> const& build) override;

    void remove_surface(
        std::shared_ptr<scene::Session> const& session,
        std::weak_ptr<scene::Surface> const& surface) override;

    void add_display(geometry::Rectangle const& area) override;

    void remove_display(geometry::Rectangle const& area) override;

    bool handle_key_event(MirKeyInputEvent const* event) override;

    bool handle_touch_event(MirTouchInputEvent const* event) override;

    bool handle_pointer_event(MirPointerInputEvent const* event) override;

    int handle_set_state(std::shared_ptr<scene::Surface> const& surface, MirSurfaceState value) override;
};
}
}

#endif /* MIR_SHELL_NULL_WINDOW_MANAGER_H_ */
