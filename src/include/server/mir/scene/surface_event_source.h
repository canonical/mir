/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_SCENE_SURFACE_EVENT_SOURCE_H_
#define MIR_SCENE_SURFACE_EVENT_SOURCE_H_

#include "mir/scene/null_surface_observer.h"
#include "mir/frontend/surface_id.h"
#include "mir/frontend/event_sink.h"

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
        Surface const& surface,
        OutputPropertiesCache const& outputs,
        std::shared_ptr<frontend::EventSink> const& event_sink);

    void attrib_changed(Surface const* surf, MirWindowAttrib attrib, int value) override;
    void resized_to(Surface const* surf, geometry::Size const& size) override;
    void moved_to(Surface const* surf, geometry::Point const& top_left) override;
    void orientation_set_to(Surface const* surf, MirOrientation orientation) override;
    void client_surface_close_requested(Surface const* surf) override;
    void keymap_changed(
        Surface const* surf,
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options) override;
    void placed_relative(Surface const* surf, geometry::Rectangle const& placement) override;
    void input_consumed(Surface const* surf, MirEvent const* event) override;
    void start_drag_and_drop(Surface const* surf, std::vector<uint8_t> const& handle) override;

private:
    frontend::SurfaceId const id;
    Surface const& surface;
    OutputPropertiesCache const& outputs;
    std::weak_ptr<OutputProperties const> last_output;
    std::shared_ptr<frontend::EventSink> const event_sink;
};
}
}

#endif // MIR_SCENE_SURFACE_EVENT_SOURCE_H_
