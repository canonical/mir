/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_MAGNIFIER_HANDLE_INDICATOR_H
#define MIRAL_MAGNIFIER_HANDLE_INDICATOR_H

#include "magnifier_controls.h"

#include "software_buffer_pool.h"
#include <mir/scene/basic_surface.h>
#include <mir/geometry/forward.h>
#include <mir/compositor/compositor_id.h>

namespace mir
{
namespace graphics { class GraphicBufferAllocator; }
namespace scene { class SceneReport; }
}

namespace miral
{
class HandleIndicator : public mir::scene::BasicSurface
{
public:
    HandleIndicator(
        mir::geometry::Rectangle const& initial_rect,
        magnifier_controls::HandleKind kind,
        mir::compositor::CompositorID capture_compositor_id,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<mir::scene::SceneReport> const& scene_report,
        std::shared_ptr<mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>> const&
            display_config_registrar);

    auto generate_renderables(mir::compositor::CompositorID id) const -> mir::graphics::RenderableList override;

private:
    miral::SoftwareBufferPool mutable pool;
    mir::compositor::CompositorID capture_compositor_id;
};
}

#endif
