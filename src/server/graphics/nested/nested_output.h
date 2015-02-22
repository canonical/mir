/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_NESTED_DETAIL_NESTED_OUTPUT_H_
#define MIR_GRAPHICS_NESTED_DETAIL_NESTED_OUTPUT_H_

#include "nested_display.h"

namespace mir
{
namespace graphics
{
namespace nested
{
class HostSurface;

namespace detail
{

class NestedOutput : public DisplayBuffer
{
public:
    NestedOutput(
        EGLDisplayHandle const& egl_display,
        std::shared_ptr<HostSurface> const& host_surface,
        geometry::Rectangle const& area,
        std::shared_ptr<input::InputDispatcher> const& input_dispatcher,
        MirPixelFormat preferred_format);

    ~NestedOutput() noexcept;

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void gl_swap_buffers() override;
    void flip() override;
    MirOrientation orientation() const override;
    bool uses_alpha() const override;

    bool post_renderables_if_optimizable(RenderableList const& renderlist) override;

    NestedOutput(NestedOutput const&) = delete;
    NestedOutput operator=(NestedOutput const&) = delete;
private:
    bool const uses_alpha_;
    EGLDisplayHandle const& egl_display;
    std::shared_ptr<HostSurface> const host_surface;
    EGLConfig const egl_config;
    EGLContextStore const egl_context;
    geometry::Rectangle const area;
    std::shared_ptr<input::InputDispatcher> const dispatcher;
    EGLSurfaceHandle const egl_surface;

    static void event_thunk(MirSurface* surface, MirEvent const* event, void* context);
    void mir_event(MirEvent const& event);
};
}
}
}
}

#endif /* MIR_GRAPHICS_NESTED_DETAIL_NESTED_OUTPUT_H_ */
