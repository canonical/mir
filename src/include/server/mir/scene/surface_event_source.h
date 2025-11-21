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

#ifndef MIR_SCENE_SURFACE_EVENT_SOURCE_H_
#define MIR_SCENE_SURFACE_EVENT_SOURCE_H_

#include <mir/scene/null_surface_observer.h>
#include <mir/frontend/surface_id.h>
#include <mir/frontend/event_sink.h>

#include <memory>

namespace mir
{
namespace scene
{
class Surface;
class OutputProperties;
class OutputPropertiesCache;

class SurfaceEventSource : public NullSurfaceObserver
{
public:
    SurfaceEventSource(
        frontend::SurfaceId id,
        OutputPropertiesCache const& outputs,
        std::shared_ptr<frontend::EventSink> const& event_sink);

    void attrib_changed(Surface const* surf, MirWindowAttrib attrib, int value) override;
    void content_resized_to(Surface const* surf, geometry::Size const& content_size) override;
    void moved_to(Surface const* surf, geometry::Point const& top_left) override;
    void orientation_set_to(Surface const* surf, MirOrientation orientation) override;
    void client_surface_close_requested(Surface const* surf) override;
    void placed_relative(Surface const* surf, geometry::Rectangle const& placement) override;
    void input_consumed(Surface const* surf, std::shared_ptr<MirEvent const> const& event) override;

private:
    frontend::SurfaceId const id;
    OutputPropertiesCache const& outputs;
    std::weak_ptr<OutputProperties const> last_output;
    std::shared_ptr<frontend::EventSink> const event_sink;
};
}
}

#endif // MIR_SCENE_SURFACE_EVENT_SOURCE_H_
