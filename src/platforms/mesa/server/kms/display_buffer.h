/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_MESA_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display.h"
#include "mir/renderer/gl/render_target.h"
#include "display_helpers.h"
#include "egl_helper.h"
#include "platform_common.h"

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
class FBHandle;
class KMSOutput;
class NativeBuffer;

class GBMOutputSurface : public renderer::gl::RenderTarget
{
public:
    class FrontBuffer
    {
    public:
        FrontBuffer();
        FrontBuffer(gbm_surface* surface);
        FrontBuffer(FrontBuffer&& from);

        ~FrontBuffer();

        FrontBuffer& operator=(FrontBuffer&& from);
        FrontBuffer& operator=(std::nullptr_t);

        operator gbm_bo*();
        operator bool() const;

    private:
        gbm_surface* const surf;
        gbm_bo* const bo;
    };

    GBMOutputSurface(
        int drm_fd,
        GBMSurfaceUPtr&& surface,
        uint32_t width,
        uint32_t height,
        helpers::EGLHelper&& egl);
    GBMOutputSurface(GBMOutputSurface&& from);

    // gl::RenderTarget
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;

    FrontBuffer lock_front();
    void report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> const& to);
    geometry::Size size() const { return {width, height}; }
private:
    int const drm_fd;
    uint32_t width, height;
    helpers::EGLHelper egl;
    GBMSurfaceUPtr surface;
};

class DisplayBuffer : public graphics::DisplayBuffer,
                      public graphics::DisplaySyncGroup,
                      public graphics::NativeDisplayBuffer,
                      public renderer::gl::RenderTarget
{
public:
    DisplayBuffer(BypassOption bypass_options,
                  std::shared_ptr<DisplayReport> const& listener,
                  std::vector<std::shared_ptr<KMSOutput>> const& outputs,
                  GBMOutputSurface&& surface_gbm,
                  geometry::Rectangle const& area,
                  glm::mat2 const& transformation);
    ~DisplayBuffer();

    geometry::Rectangle view_area() const override;
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    bool overlay(RenderableList const& renderlist) override;
    void bind() override;

    void for_each_display_buffer(
        std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    glm::mat2 transformation() const override;
    NativeDisplayBuffer* native_display_buffer() override;

    void set_transformation(glm::mat2 const& t, geometry::Rectangle const& a);
    void schedule_set_crtc();
    void wait_for_page_flip();

private:
    bool schedule_page_flip(FBHandle const& bufobj);
    void set_crtc(FBHandle const&);

    std::shared_ptr<graphics::Buffer> visible_bypass_frame, scheduled_bypass_frame;
    std::shared_ptr<Buffer> bypass_buf{nullptr};
    FBHandle* bypass_bufobj{nullptr};
    std::shared_ptr<DisplayReport> const listener;
    BypassOption bypass_option;

    std::vector<std::shared_ptr<KMSOutput>> outputs;

    /*
     * Destruction order is important here:
     *  - The GBMFrontBuffers depend on *either*:
     *  i)  The GBMOutputSurface, or
     *  ii) The EGLBufferCopier hidden inside get_front_buffer
     */
    std::function<GBMOutputSurface::FrontBuffer(GBMOutputSurface::FrontBuffer&&)> get_front_buffer;
    GBMOutputSurface surface;

    GBMOutputSurface::FrontBuffer visible_composite_frame;
    GBMOutputSurface::FrontBuffer scheduled_composite_frame;

    geometry::Rectangle area;
    glm::mat2 transform;
    std::atomic<bool> needs_set_crtc;
    std::chrono::milliseconds recommend_sleep{0};
    bool page_flips_pending;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_DISPLAY_BUFFER_H_ */
