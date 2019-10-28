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
 */

#ifndef MIR_PLATFORMS_WAYLAND_DISPLAY_H_
#define MIR_PLATFORMS_WAYLAND_DISPLAY_H_

#include "displayclient.h"
#include <mir/graphics/display.h>
#include <mir/graphics/display_report.h>
#include <mir/renderer/gl/context_source.h>

namespace mir
{
namespace graphics
{
namespace wayland
{
class Display : public mir::graphics::Display, public mir::graphics::NativeDisplay,
                public mir::renderer::gl::ContextSource, DisplayClient
{
public:
    Display(
        wl_display* const wl_display,
        std::shared_ptr<GLConfig> const& gl_config,
        std::shared_ptr<DisplayReport> const& report);

    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f) override;

    auto configuration() const -> std::unique_ptr<DisplayConfiguration> override;

    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;

    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler, DisplayResumeHandler const& resume_handler) override;

    void pause() override;

    void resume() override;

    auto create_hardware_cursor() -> std::shared_ptr<Cursor>override;

    auto create_virtual_output(int width, int height) -> std::unique_ptr<VirtualOutput> override;

    NativeDisplay* native_display() override;

    auto last_frame_on(unsigned output_id) const -> Frame override;

    auto create_gl_context() const -> std::unique_ptr<mir::renderer::gl::Context> override;

private:
    std::shared_ptr<DisplayReport> const report;
};

}
}

}

#endif  // MIR_PLATFORMS_WAYLAND_DISPLAY_H_
