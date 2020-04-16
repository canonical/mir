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

#ifndef MIR_SHELL_SURFACE_READY_OBSERVER_H_
#define MIR_SHELL_SURFACE_READY_OBSERVER_H_

#include "mir/scene/null_surface_observer.h"

#include <functional>
#include <memory>

namespace mir
{
namespace scene { class Session; class Surface; }
namespace geometry { struct Size; }

namespace shell
{
class SurfaceReadyObserver : public scene::NullSurfaceObserver,
    public std::enable_shared_from_this<SurfaceReadyObserver>
{
public:
    using ActivateFunction = std::function<void(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface)>;

    SurfaceReadyObserver(
        ActivateFunction const& activate,
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface);

    ~SurfaceReadyObserver();

private:
    void frame_posted(scene::Surface const* surf, int, geometry::Size const&) override;

    ActivateFunction const activate;
    std::weak_ptr<scene::Session> const session;
    std::weak_ptr<scene::Surface> const surface;
};
}
}

#endif /* MIR_SHELL_SURFACE_READY_OBSERVER_H_ */
