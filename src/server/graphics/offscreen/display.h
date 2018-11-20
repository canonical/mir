/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_OFFSCREEN_DISPLAY_H_
#define MIR_GRAPHICS_OFFSCREEN_DISPLAY_H_

#include "mir/graphics/display.h"
#include "display_configuration.h"
#include "mir/graphics/surfaceless_egl_context.h"
#include "mir/renderer/gl/context_source.h"

#include <mutex>
#include <vector>

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{

class DisplayConfigurationPolicy;
class DisplayReport;

namespace offscreen
{
namespace detail
{

class EGLDisplayHandle
{
public:
    explicit EGLDisplayHandle(EGLNativeDisplayType native_type);
    EGLDisplayHandle(EGLDisplayHandle&&);
    ~EGLDisplayHandle() noexcept;

    void initialize();
    operator EGLDisplay() const { return egl_display; }

private:
    EGLDisplayHandle(EGLDisplayHandle const&) = delete;
    EGLDisplayHandle operator=(EGLDisplayHandle const&) = delete;

    EGLDisplay egl_display;
};

class DisplaySyncGroup : public graphics::DisplaySyncGroup
{
public:
    DisplaySyncGroup(std::unique_ptr<DisplayBuffer> output);
    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const&) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;
private:
    std::unique_ptr<DisplayBuffer> const output;
};

}

class Display : public graphics::Display,
                public graphics::NativeDisplay,
                public renderer::gl::ContextSource
{
public:
    Display(EGLNativeDisplayType egl_native_display,
            std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
            std::shared_ptr<DisplayReport> const& listener);
    ~Display() noexcept;

    void for_each_display_sync_group(std::function<void(DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<graphics::DisplayConfiguration> configuration() const override;
    void configure(graphics::DisplayConfiguration const& conf) override;

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
    Frame last_frame_on(unsigned output_id) const override;

    std::unique_ptr<renderer::gl::Context> create_gl_context() const override;
    bool apply_if_configuration_preserves_display_buffers(graphics::DisplayConfiguration const& conf) override;
private:
    detail::EGLDisplayHandle const egl_display;
    SurfacelessEGLContext const egl_context_shared;
    mutable std::mutex configuration_mutex;
    DisplayConfiguration current_display_configuration;
    std::vector<std::unique_ptr<DisplaySyncGroup>> display_sync_groups;
};

}
}
}

#endif /* MIR_GRAPHICS_OFFSCREEN_DISPLAY_H_ */
