/*
 * Copyright © 2014 Canonical Ltd.
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
class SurfaceEventSource : public NullSurfaceObserver
{
public:
    SurfaceEventSource(
        frontend::SurfaceId id,
        std::shared_ptr<frontend::EventSink> const& event_sink);

    void attrib_changed(MirSurfaceAttrib attrib, int value) override;
    void resized_to(geometry::Size const& size) override;
    void orientation_set_to(MirOrientation orientation) override;
    void client_surface_close_requested() override;
    void keymap_changed(xkb_rule_names const& names) override;

private:
    frontend::SurfaceId const id;
    std::shared_ptr<frontend::EventSink> const event_sink;
};
}
}

#endif // MIR_SCENE_SURFACE_EVENT_SOURCE_H_
