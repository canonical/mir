/*
 * Copyright © 2021 Canonical Ltd.
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
 */
#include <mir_toolkit/common.h>
#include <mir/geometry/size.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/cursor_image.h>
#include <glm/glm.hpp>
#include <mir_toolkit/mir_input_device_types.h>
#include <vector>

namespace mir { namespace scene { class SurfaceObserver; } }
namespace mir { namespace scene { class Surface; } }

struct MirEvent;
struct MirInputEvent;

namespace miroil 
{
    
class SurfaceObserver
{
public:
    SurfaceObserver() = default;
    SurfaceObserver(SurfaceObserver const&) = delete;
    SurfaceObserver& operator=(SurfaceObserver const&) = delete;    
    virtual ~SurfaceObserver() = default;
    
    virtual void attrib_changed(mir::scene::Surface const* surf, MirWindowAttrib attrib, int value) = 0;
    virtual void window_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& window_size) = 0;
    virtual void content_resized_to(mir::scene::Surface const* surf, mir::geometry::Size const& content_size) = 0;
    virtual void moved_to(mir::scene::Surface const* surf, mir::geometry::Point const& top_left) = 0;
    virtual void hidden_set_to(mir::scene::Surface const* surf, bool hide) = 0;
    virtual void frame_posted(mir::scene::Surface const* surf, int frames_available, mir::geometry::Size const& size) = 0;
    virtual void alpha_set_to(mir::scene::Surface const* surf, float alpha) = 0;
    virtual void orientation_set_to(mir::scene::Surface const* surf, MirOrientation orientation) = 0;
    virtual void transformation_set_to(mir::scene::Surface const* surf, glm::mat4 const& t) = 0;
    virtual void cursor_image_set_to(mir::scene::Surface const* surf, mir::graphics::CursorImage const& image) = 0;
    virtual void client_surface_close_requested(mir::scene::Surface const* surf) = 0;
    virtual void keymap_changed(mir::scene::Surface const* surf, MirInputDeviceId id, std::string const& model,
                                std::string const& layout, std::string const& variant, std::string const& options) = 0;
    virtual void renamed(mir::scene::Surface const* surf, char const* name) = 0;
    virtual void cursor_image_removed(mir::scene::Surface const* surf) = 0;
    virtual void placed_relative(mir::scene::Surface const* surf, mir::geometry::Rectangle const& placement) = 0;
    virtual void input_consumed(mir::scene::Surface const* surf, MirEvent const* event) = 0;
    virtual void start_drag_and_drop(mir::scene::Surface const* surf, std::vector<uint8_t> const& handle) = 0;
    virtual void depth_layer_set_to(mir::scene::Surface const* surf, MirDepthLayer depth_layer) = 0;
    virtual void application_id_set_to(mir::scene::Surface const* surf, std::string const& application_id) = 0;
};

}
