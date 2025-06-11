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

#ifndef MIR_GRAPHICS_X_DISPLAY_SINK_H_
#define MIR_GRAPHICS_X_DISPLAY_SINK_H_

#include "mir/graphics/display_sink.h"
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

class DisplaySink : public graphics::DisplaySink,
                      public graphics::DisplaySyncGroup
{
public:
    DisplaySink(
            xcb_connection_t* connection,
            xcb_window_t win,
            std::shared_ptr<helpers::EGLHelper> egl,
            geometry::Rectangle const& view_area,
            std::shared_ptr<DisplayReport> const& report);

    ~DisplaySink();

    auto view_area() const -> geometry::Rectangle override;

    auto overlay(std::vector<DisplayElement> const& renderlist) -> bool override;
    void set_next_image(std::unique_ptr<Framebuffer> content) override;

    glm::mat2 transformation() const override;

protected:
    auto maybe_create_allocator(DisplayAllocator::Tag const& type_tag) -> DisplayAllocator* override;

public:

    void set_view_area(geometry::Rectangle const& a);
    void set_transformation(glm::mat2 const& t);

    void for_each_display_sink(
        std::function<void(graphics::DisplaySink&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    auto x11_window() const -> xcb_window_t;
private:
    class Allocator;
    std::unique_ptr<Allocator> egl_allocator;

    std::shared_ptr<helpers::Framebuffer> next_frame;
    geometry::Rectangle area;
    geometry::Size in_pixels;
    glm::mat2 transform;
    std::shared_ptr<helpers::EGLHelper> egl;
    xcb_connection_t* const x11_connection;
    xcb_window_t const x_win;
    std::shared_ptr<DisplayReport> const report;
};

}
}
}

#endif /* MIR_GRAPHICS_X_DISPLAY_SINK_H_ */
