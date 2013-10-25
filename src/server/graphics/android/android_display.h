/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/egl_resources.h"
#include "android_display_configuration.h"

#include <EGL/egl.h>
#include <memory>

namespace mir
{
namespace graphics
{

class DisplayReport;
class DisplayBuffer;

namespace android
{
class DisplayDevice;

class AndroidDisplayBufferFactory;
class DisplaySupportProvider;

class AndroidDisplay : public Display
{
public:
    explicit AndroidDisplay(std::shared_ptr<ANativeWindow> const&,
                            std::shared_ptr<AndroidDisplayBufferFactory> const& db_factory,
                            std::shared_ptr<DisplayDevice> const& display_device,
                            std::shared_ptr<DisplayReport> const& display_report);
    ~AndroidDisplay();

    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f);

    std::shared_ptr<DisplayConfiguration> configuration();
    void configure(DisplayConfiguration const&);

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler);

    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler);

    void pause();
    void resume();

    std::weak_ptr<Cursor> the_cursor();
    std::unique_ptr<graphics::GLContext> create_gl_context();

private:
    std::shared_ptr<ANativeWindow> const native_window;
    std::shared_ptr<DisplayDevice> const display_device;

    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContextStore const egl_context_shared;
    EGLSurfaceStore const egl_surface_dummy;
    std::unique_ptr<DisplayBuffer> display_buffer;
    
    AndroidDisplayConfiguration current_configuration;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_ */
