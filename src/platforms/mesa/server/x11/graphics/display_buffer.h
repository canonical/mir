/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_X_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/render_target.h"
#include "egl_helper.h"

#include <EGL/egl.h>
#include <memory>

namespace mir
{
namespace graphics
{

class AtomicFrame;
class GLConfig;
class DisplayReport;

namespace X
{

class DisplayBuffer : public graphics::DisplayBuffer,
                      public graphics::DisplaySyncGroup,
                      public graphics::NativeDisplayBuffer,
                      public renderer::gl::RenderTarget
{
public:
    DisplayBuffer(
            ::Display* const x_dpy,
            Window const win,
            geometry::Size const sz,
            EGLContext const shared_context,
            std::shared_ptr<AtomicFrame> const& f,
            std::shared_ptr<DisplayReport> const& r,
            MirOrientation const o,
            GLConfig const& gl_config);

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;
    bool overlay(RenderableList const& renderlist) override;
    void set_orientation(MirOrientation const new_orientation);

    void for_each_display_buffer(
        std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    MirOrientation orientation() const override;
    MirMirrorMode mirror_mode() const override;
    NativeDisplayBuffer* native_display_buffer() override;

private:
    geometry::Size const size;
    std::shared_ptr<DisplayReport> const report;
    MirOrientation orientation_;
    helpers::EGLHelper egl;
    std::shared_ptr<AtomicFrame> const last_frame;

    typedef EGLBoolean (EGLAPIENTRY EglGetSyncValuesCHROMIUM)
        (EGLDisplay dpy, EGLSurface surface, int64_t *ust,
         int64_t *msc, int64_t *sbc);
    EglGetSyncValuesCHROMIUM* eglGetSyncValues;
};

}
}
}

#endif /* MIR_GRAPHICS_X_DISPLAY_BUFFER_H_ */
