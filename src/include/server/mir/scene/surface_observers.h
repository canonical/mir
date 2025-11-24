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

#ifndef MIR_SCENE_SURFACE_OBSERVERS_H_
#define MIR_SCENE_SURFACE_OBSERVERS_H_

#include <mir/observer_multiplexer.h>
#include <mir/scene/surface_observer.h>

namespace mir
{
namespace scene
{

class SurfaceObservers : public ObserverMultiplexer<SurfaceObserver>
{
public:
    SurfaceObservers()
        : ObserverMultiplexer{immediate_executor}
    {
    }

    void attrib_changed(Surface const* surf, MirWindowAttrib attrib, int value) override;
    void window_resized_to(Surface const* surf, geometry::Size const& window_size) override;
    void content_resized_to(Surface const* surf, geometry::Size const& content_size) override;
    void moved_to(Surface const* surf, geometry::Point const& top_left) override;
    void hidden_set_to(Surface const* surf, bool hide) override;
    void frame_posted(Surface const* surf, int frames_available, geometry::Rectangle const& damage) override;
    void alpha_set_to(Surface const* surf, float alpha) override;
    void orientation_set_to(Surface const* surf, MirOrientation orientation) override;
    void transformation_set_to(Surface const* surf, glm::mat4 const& t) override;
    void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) override;
    void cursor_image_set_to(Surface const* surf, std::weak_ptr<mir::graphics::CursorImage> const& image) override;
    void client_surface_close_requested(Surface const* surf) override;
    void renamed(Surface const* surf, std::string const&) override;
    void cursor_image_removed(Surface const* surf) override;
    void placed_relative(Surface const* surf, geometry::Rectangle const& placement) override;
    void input_consumed(Surface const* surf, std::shared_ptr<MirEvent const> const& event) override;
    void depth_layer_set_to(Surface const* surf, MirDepthLayer depth_layer) override;
    void application_id_set_to(Surface const* surf, std::string const& application_id) override;
};

}
}

#endif /* MIR_SCENE_SURFACE_OBSERVERS_H_ */
