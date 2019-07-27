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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SCENE_NULL_SURFACE_OBSERVER_H_
#define MIR_SCENE_NULL_SURFACE_OBSERVER_H_

#include "mir/scene/surface_observer.h"

namespace mir
{
namespace scene
{
class NullSurfaceObserver : public SurfaceObserver
{
public:
    NullSurfaceObserver() = default;
    virtual ~NullSurfaceObserver() = default;

    void attrib_changed(Surface const* surf, MirWindowAttrib attrib, int value) override;
    void resized_to(Surface const* surf, geometry::Size const& size) override;
    void moved_to(Surface const* surf, geometry::Point const& top_left) override;
    void hidden_set_to(Surface const* surf, bool hide) override;
    void frame_posted(Surface const* surf, int frames_available, geometry::Size const& size) override;
    void alpha_set_to(Surface const* surf, float alpha) override;
    void orientation_set_to(Surface const* surf, MirOrientation orientation) override;
    void transformation_set_to(Surface const* surf, glm::mat4 const& t) override;
    void cursor_image_set_to(Surface const* surf, graphics::CursorImage const& image) override;
    void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) override;
    void client_surface_close_requested(Surface const* surf) override;
    void keymap_changed(
        Surface const* surf,
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options) override;
    void renamed(Surface const* surf, char const* name) override;
    void cursor_image_removed(Surface const* surf) override;
    void placed_relative(Surface const* surf, geometry::Rectangle const& placement) override;
    void input_consumed(Surface const* surf, MirEvent const* event) override;
    void start_drag_and_drop(Surface const* surf, std::vector<uint8_t> const& handle) override;
    void depth_layer_set_to(Surface const* surf, MirDepthLayer depth_layer) override;
    void application_id_set_to(Surface const* surf, std::string const& application_id) override;
    void session_set_to(Surface const* surf, std::weak_ptr<Session> const& session) override;

protected:
    NullSurfaceObserver(NullSurfaceObserver const&) = delete;
    NullSurfaceObserver& operator=(NullSurfaceObserver const&) = delete;
};
}
}

#endif // MIR_SCENE_NULL_SURFACE_OBSERVER_H_
