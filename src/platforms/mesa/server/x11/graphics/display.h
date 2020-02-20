/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_H_
#define MIR_GRAPHICS_X_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/geometry/size.h"
#include "mir/renderer/gl/context_source.h"
#include "mir_toolkit/common.h"
#include "egl_helper.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>

#include <memory>

namespace mir
{
namespace graphics
{

class AtomicFrame;
class GLConfig;
class DisplayReport;
struct DisplayConfigurationOutput;

namespace X
{

class DisplayBuffer;
class X11OutputConfig;

class X11Window
{
public:
    X11Window(::Display* const x_dpy,
              EGLDisplay egl_dpy,
              geometry::Size const size,
              EGLConfig const egl_cfg);
    ~X11Window();

    operator Window() const;
    unsigned long red_mask() const;

private:
    ::Display* const x_dpy;
    Window win;
    unsigned long r_mask;
};

class Display : public graphics::Display,
                public graphics::NativeDisplay,
                public renderer::gl::ContextSource
{
public:
    explicit Display(::Display* x_dpy,
                     std::vector<X11OutputConfig> const& requested_size,
                     std::shared_ptr<GLConfig> const& gl_config,
                     std::shared_ptr<DisplayReport> const& report);
    ~Display() noexcept;

    void for_each_display_sync_group(std::function<void(graphics::DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<graphics::DisplayConfiguration> configuration() const override;

    bool apply_if_configuration_preserves_display_buffers(graphics::DisplayConfiguration const& conf) override;

    void configure(graphics::DisplayConfiguration const&) override;

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler) override;

    void pause() override;
    void resume() override;

    std::shared_ptr<Cursor> create_hardware_cursor() override;
    std::unique_ptr<VirtualOutput> create_virtual_output(int width, int height) override;

    NativeDisplay* native_display() override;

    std::unique_ptr<renderer::gl::Context> create_gl_context() const override;

    Frame last_frame_on(unsigned output_id) const override;

private:
    struct OutputInfo
    {
        OutputInfo(
            std::unique_ptr<X11Window> window,
            std::unique_ptr<DisplayBuffer> display_buffer,
            std::shared_ptr<DisplayConfigurationOutput> configuration);
        ~OutputInfo();

        std::unique_ptr<X11Window> window;
        std::unique_ptr<DisplayBuffer> display_buffer;
        std::shared_ptr<DisplayConfigurationOutput> configuration;
    };

    std::vector<std::unique_ptr<OutputInfo>> outputs;
    helpers::EGLHelper shared_egl;
    ::Display* const x_dpy;
    std::shared_ptr<GLConfig> const gl_config;
    float pixel_width;
    float pixel_height;
    std::shared_ptr<DisplayReport> const report;
    std::shared_ptr<AtomicFrame> last_frame;
};

}

}
}
#endif /* MIR_GRAPHICS_X_DISPLAY_H_ */
