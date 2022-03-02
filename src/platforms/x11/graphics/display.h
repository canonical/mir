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
#include "mir/geometry/size_f.h"
#include "mir/renderer/gl/context_source.h"
#include "mir_toolkit/common.h"
#include "egl_helper.h"
#include "../x11_resources.h"

#include <EGL/egl.h>

#include <memory>
#include <map>

namespace mir
{
namespace graphics
{

class AtomicFrame;
class GLConfig;
class DisplayReport;
struct DisplayConfigurationOutput;
class DisplayConfigurationPolicy;

namespace X
{

class DisplayBuffer;
class X11OutputConfig;

class X11Window
{
public:
    X11Window(mir::X::X11Resources* x11_resources,
              std::string title,
              EGLDisplay egl_dpy,
              geometry::Size const size,
              EGLConfig const egl_cfg);
    ~X11Window();

    operator xcb_window_t() const;
    unsigned long red_mask() const;

private:
    mir::X::X11Resources* const x11_resources;
    std::string const title;
    xcb_window_t win;
    unsigned long r_mask;
};

class Display : public graphics::Display
{
public:
    explicit Display(std::shared_ptr<mir::X::X11Resources> const& x11_resources,
                     std::string const title,
                     std::vector<X11OutputConfig> const& requested_size,
                     std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
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

    std::unique_ptr<renderer::gl::Context> create_gl_context() const override;

    Frame last_frame_on(unsigned output_id) const override;

private:
    struct OutputInfo : ::mir::X::X11Resources::VirtualOutput
    {
        OutputInfo(
            Display* owner,
            std::unique_ptr<X11Window> window,
            std::unique_ptr<DisplayBuffer> display_buffer,
            std::shared_ptr<DisplayConfigurationOutput> configuration);
        ~OutputInfo();

        auto configuration() const -> graphics::DisplayConfigurationOutput const& override { return *config; }
        void set_size(geometry::Size const& size) override;

        Display* const owner;
        std::unique_ptr<X11Window> const window;
        std::unique_ptr<DisplayBuffer> const display_buffer;
        std::shared_ptr<DisplayConfigurationOutput> const config;
    };

    helpers::EGLHelper const shared_egl;
    std::shared_ptr<mir::X::X11Resources> const x11_resources;
    std::shared_ptr<GLConfig> const gl_config;
    geometry::SizeF pixel_size_mm;
    std::shared_ptr<DisplayReport> const report;
    std::shared_ptr<AtomicFrame> const last_frame;

    std::mutex mutable mutex;
    std::vector<std::unique_ptr<OutputInfo>> outputs;
    std::vector<DisplayConfigurationChangeHandler> config_change_handlers;
};

}

}
}
#endif /* MIR_GRAPHICS_X_DISPLAY_H_ */
