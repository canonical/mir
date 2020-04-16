/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_WINDOW_MANAGEMENT_INFO_H
#define MIR_SHELL_WINDOW_MANAGEMENT_INFO_H

#include "mir/geometry/rectangles.h"
#include "mir/optional_value.h"
#include "mir/shell/surface_specification.h"
#include "mir/frontend/surface_id.h"
#include "mir/frontend/buffer_stream_id.h"

#include <vector>

namespace mir
{
namespace scene { class Session; class Surface; struct SurfaceCreationParameters; }
namespace shell
{
struct SurfaceInfo
{
    SurfaceInfo(
        std::shared_ptr <scene::Session> const& session,
        std::shared_ptr <scene::Surface> const& surface,
        scene::SurfaceCreationParameters const& params);

    bool can_be_active() const;

    bool can_morph_to(MirWindowType new_type) const;

    bool must_have_parent() const;

    bool must_not_have_parent() const;

    bool is_visible() const;

    static bool needs_titlebar(MirWindowType type);

    void constrain_resize(
        std::shared_ptr <scene::Surface> const& surface,
        geometry::Point& requested_pos,
        geometry::Size& requested_size,
        const bool left_resize,
        const bool top_resize,
        geometry::Rectangle const& bounds) const;

    MirWindowType type;
    MirWindowState state;
    geometry::Rectangle restore_rect;
    std::weak_ptr <scene::Session> session;
    std::weak_ptr <scene::Surface> parent;
    std::vector <std::weak_ptr<scene::Surface>> children;
    std::shared_ptr <scene::Surface> titlebar;
    frontend::SurfaceId titlebar_id;
    frontend::BufferStreamId titlebar_stream_id;
    bool is_titlebar = false;
    geometry::Width min_width;
    geometry::Height min_height;
    geometry::Width max_width;
    geometry::Height max_height;
    mir::optional_value<geometry::DeltaX> width_inc;
    mir::optional_value<geometry::DeltaY> height_inc;
    mir::optional_value<shell::SurfaceAspectRatio> min_aspect;
    mir::optional_value<shell::SurfaceAspectRatio> max_aspect;
    mir::optional_value<graphics::DisplayConfigurationOutputId> output_id;
    mir::optional_value<MirPointerConfinementState> confine_pointer;
};

struct SessionInfo
{
    std::vector<std::weak_ptr<scene::Surface>> surfaces;

    // This is only used by the TilingWindowManagerPolicy,
    // perhaps we need a more extensible mechanism. (std::experimental::any?)
    geometry::Rectangle tile;
};
}
}

#endif //MIR_SHELL_WINDOW_MANAGEMENT_INFO_H
