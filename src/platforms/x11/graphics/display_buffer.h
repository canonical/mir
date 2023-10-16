/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_X_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/gl/render_target.h"
#include "egl_helper.h"

#include <EGL/egl.h>
#include <memory>

namespace mir
{
namespace graphics
{

class GLConfig;
class DisplayReport;

namespace X
{
class Platform;

class DisplayBuffer : public graphics::DisplayBuffer,
                      public graphics::DisplaySyncGroup
{
public:
    DisplayBuffer(
            std::shared_ptr<Platform> parent,
            xcb_window_t win,
            geometry::Rectangle const& view_area);

    auto view_area() const -> geometry::Rectangle override;

    auto overlay(std::vector<DisplayElement> const& renderlist) -> bool override;
    void set_next_image(std::unique_ptr<Framebuffer> content) override;

    glm::mat2 transformation() const override;

    auto target() const -> std::shared_ptr<DisplayTarget> override;

    void set_view_area(geometry::Rectangle const& a);
    void set_transformation(glm::mat2 const& t);

    void for_each_display_buffer(
        std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    auto x11_window() const -> xcb_window_t;
private:
    std::shared_ptr<Platform> const parent;
    std::shared_ptr<helpers::Framebuffer> next_frame;
    geometry::Rectangle area;
    glm::mat2 transform;
    xcb_window_t const x_win;
};

}
}
}

#endif /* MIR_GRAPHICS_X_DISPLAY_BUFFER_H_ */
