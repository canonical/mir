/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_MESA_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/render_target.h"
#include "display_helpers.h"

#include <vector>
#include <memory>
#include <atomic>

namespace mir
{
namespace graphics
{

class DisplayReport;
class GLConfig;

namespace mesa
{

class Platform;
class BufferObject;
class KMSOutput;

class DisplayBuffer : public graphics::DisplayBuffer,
                      public graphics::DisplaySyncGroup,
                      public graphics::NativeDisplayBuffer,
                      public renderer::gl::RenderTarget
{
public:
    DisplayBuffer(std::shared_ptr<Platform> const& platform,
                  std::shared_ptr<DisplayReport> const& listener,
                  std::vector<std::shared_ptr<KMSOutput>> const& outputs,
                  GBMSurfaceUPtr surface_gbm,
                  geometry::Rectangle const& area,
                  MirOrientation rot,
                  GLConfig const& gl_config,
                  EGLContext shared_context);
    ~DisplayBuffer();

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    bool post_renderables_if_optimizable(RenderableList const& renderlist) override;

    void for_each_display_buffer(
        std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    MirOrientation orientation() const override;
    NativeDisplayBuffer* native_display_buffer() override;

    void set_orientation(MirOrientation const rot, geometry::Rectangle const& a);
    void schedule_set_crtc();
    void wait_for_page_flip();

private:
    BufferObject* get_front_buffer_object();
    BufferObject* get_buffer_object(struct gbm_bo *bo);
    bool schedule_page_flip(BufferObject* bufobj);
    void set_crtc(BufferObject const*);

    BufferObject* visible_composite_frame;
    BufferObject* scheduled_composite_frame;
    std::shared_ptr<graphics::Buffer> visible_bypass_frame, scheduled_bypass_frame;
    std::shared_ptr<Buffer> bypass_buf{nullptr};
    BufferObject* bypass_bufobj{nullptr};
    std::shared_ptr<Platform> const platform;
    std::shared_ptr<DisplayReport> const listener;
    /* DRM helper from mgm::Platform */
    helpers::DRMHelper& drm;
    std::vector<std::shared_ptr<KMSOutput>> outputs;
    GBMSurfaceUPtr surface_gbm;
    helpers::EGLHelper egl;
    geometry::Rectangle area;
    uint32_t fb_width, fb_height;
    MirOrientation rotation;
    std::atomic<bool> needs_set_crtc;
    std::chrono::milliseconds recommend_sleep{0};
    bool page_flips_pending;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_DISPLAY_BUFFER_H_ */
