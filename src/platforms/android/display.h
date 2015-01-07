/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_H_

#include "mir/graphics/display.h"
#include "gl_context.h"
#include "hwc_configuration.h"
#include "overlay_optimization.h"

#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{

class DisplayReport;
class GLConfig;
class GLProgramFactory;

namespace android
{
class DisplayComponentFactory;
class DisplaySupportProvider;
class ConfigurableDisplayBuffer;

class Display : public graphics::Display
{
public:
    explicit Display(
        std::shared_ptr<DisplayComponentFactory> const& display_buffer_builder,
        std::shared_ptr<GLProgramFactory> const& gl_program_factory,
        std::shared_ptr<GLConfig> const& gl_config,
        std::shared_ptr<DisplayReport> const& display_report,
        OverlayOptimization overlay_option);
    ~Display() noexcept;

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override;

    std::unique_ptr<graphics::DisplayConfiguration> configuration() const override;
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

    std::shared_ptr<Cursor> create_hardware_cursor(std::shared_ptr<CursorImage> const& initial_image) override;
    std::unique_ptr<graphics::GLContext> create_gl_context() override;

private:
    std::shared_ptr<DisplayComponentFactory> const display_buffer_builder;
    std::unique_ptr<HwcConfiguration> const hwc_config;
    DisplayAttribs attribs;
    PbufferGLContext gl_context;
    mutable std::mutex configuration_mutex;

    //primary display is always connected
    std::unique_ptr<ConfigurableDisplayBuffer> const display_buffer;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_H_ */
