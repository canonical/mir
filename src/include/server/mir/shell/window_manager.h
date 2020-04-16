/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_WINDOW_MANAGER_H_
#define MIR_SHELL_WINDOW_MANAGER_H_

#include "mir/frontend/surface_id.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/event.h"

#include <memory>

namespace mir
{
namespace geometry { struct Rectangle; }
namespace scene { class Session; class Surface; struct SurfaceCreationParameters; }
namespace shell
{
struct SurfaceSpecification;

/// interface to provide window management logic
class WindowManager
{
public:
    virtual void add_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual void remove_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual auto add_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::function<std::shared_ptr<scene::Surface>(
            std::shared_ptr<scene::Session> const& session,
            scene::SurfaceCreationParameters const& params)> const& build) -> std::shared_ptr<scene::Surface> = 0;

    virtual void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        SurfaceSpecification  const& modifications) = 0;

    virtual void remove_surface(
        std::shared_ptr<scene::Session> const& session,
        std::weak_ptr<scene::Surface> const& surface) = 0;

    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;

    virtual bool handle_keyboard_event(MirKeyboardEvent const* event) = 0;

    virtual bool handle_touch_event(MirTouchEvent const* event) = 0;

    virtual bool handle_pointer_event(MirPointerEvent const* event) = 0;

    virtual int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirWindowAttrib attrib,
        int value) = 0;

    virtual void handle_raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) = 0;

    virtual void handle_request_drag_and_drop(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) = 0;

    virtual void handle_request_move(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) = 0;

    virtual void handle_request_resize(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp,
        MirResizeEdge edge) = 0;

    virtual ~WindowManager() = default;
    WindowManager() = default;
    WindowManager(WindowManager const&) = delete;
    WindowManager& operator=(WindowManager const&) = delete;
};
}
}

#endif /* MIR_SHELL_WINDOW_MANAGER_H_ */
