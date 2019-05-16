/*
 * Copyright © 2013-2019 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_FOCUS_CONTROLLER_H_
#define MIR_SHELL_FOCUS_CONTROLLER_H_

#include <stddef.h>
#include <memory>
#include <set>
#include <vector>

namespace mir
{
namespace geometry { struct Point; }
namespace scene { class Session; class Surface; }

namespace shell
{
using SurfaceSet = std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>>;

class FocusController
{
public:
    virtual ~FocusController() = default;

    virtual void focus_next_session() = 0;
    virtual void focus_prev_session() = 0;

    virtual auto focused_session() const -> std::shared_ptr<scene::Session> = 0;

    virtual void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) = 0;

    virtual std::shared_ptr<scene::Surface> focused_surface() const = 0;

    virtual auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> = 0;

    virtual void raise(SurfaceSet const& surfaces) = 0;

    virtual void set_drag_and_drop_handle(std::vector<uint8_t> const& handle) = 0;
    virtual void clear_drag_and_drop_handle() = 0;

protected:
    FocusController() = default;
    FocusController(FocusController const&) = delete;
    FocusController& operator=(FocusController const&) = delete;
};

}
} // namespace mir

#endif // MIR_SHELL_FOCUS_CONTROLLER_H_
