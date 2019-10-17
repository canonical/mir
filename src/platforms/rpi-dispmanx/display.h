/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_DISPLAY_H
#define MIR_DISPLAY_H

#include "mir/graphics/display.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/geometry/size.h"

#include <EGL/egl.h>

#include <bcm_host.h>
#include <mir/graphics/gl_config.h>

namespace mir
{
namespace graphics
{
namespace rpi
{
class DisplayBuffer;

class Display
    : public graphics::Display,
      public graphics::NativeDisplay,
      public renderer::gl::ContextSource
{
public:
    Display(EGLDisplay dpy, GLConfig const& gl_config, uint32_t device);
    ~Display() noexcept;

    void for_each_display_sync_group(std::function<void(DisplaySyncGroup&)> const& f) override;
    std::unique_ptr<graphics::DisplayConfiguration> configuration() const override;
    bool apply_if_configuration_preserves_display_buffers(graphics::DisplayConfiguration const& conf) override;
    void configure(graphics::DisplayConfiguration const& conf) override;
    void register_configuration_change_handler(
        EventHandlerRegister& handlers, DisplayConfigurationChangeHandler const& conf_change_handler) override;
    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler) override;
    void pause() override;
    void resume() override;
    std::shared_ptr<Cursor> create_hardware_cursor() override;
    std::unique_ptr<VirtualOutput> create_virtual_output(int width, int height) override;
    NativeDisplay* native_display() override;
    Frame last_frame_on(unsigned output_id) const override;

    auto create_gl_context() const -> std::unique_ptr<renderer::gl::Context> override;

private:
    DISPMANX_DISPLAY_HANDLE_T const disp_handle;
    EGLDisplay dpy;
    EGLConfig egl_config;
    EGLContext ctx;

    class DisplayConfiguration;
    std::unique_ptr<DisplayConfiguration> display_config;

    std::unique_ptr<rpi::DisplayBuffer> db;

    static auto size_from_config(DisplayConfiguration const& config) -> geometry::Size;
};
}
}
}
#endif  // MIR_DISPLAY_H
