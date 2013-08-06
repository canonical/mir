/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_NESTED_DISPLAY_H_
#define MIR_GRAPHICS_NESTED_NESTED_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/nested/nested_platform.h"
#include "mir/main_loop.h"
#include "mir_toolkit/mir_client_library.h"

#include "nested_display_configuration.h"

#include <EGL/egl.h>

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace graphics
{
class DisplayReport;
class DisplayBuffer;
namespace nested
{

class NestedDisplay : public Display, public DisplayBuffer
{
public:
    NestedDisplay(std::shared_ptr<NestedPlatform>const& platform,
            std::shared_ptr<DisplayReport>const& display_report);
    virtual ~NestedDisplay() noexcept;

    geometry::Rectangle view_area() const;
    void post_update();
    void for_each_display_buffer(std::function<void(DisplayBuffer&)>const& f);

    std::shared_ptr<DisplayConfiguration> configuration();
    void configure(DisplayConfiguration const&);

    void register_configuration_change_handler(
            EventHandlerRegister& handlers,
            DisplayConfigurationChangeHandler const& conf_change_handler);

    void register_pause_resume_handlers(
            EventHandlerRegister& handlers,
            DisplayPauseHandler const& pause_handler,
            DisplayResumeHandler const& resume_handler);

    void register_configuration_change_handler(
            MainLoop& main_loop,
            DisplayConfigurationChangeHandler const& conf_change_handler);

    void pause();
    void resume();

    void make_current();
    void release_current();

    std::weak_ptr<Cursor> the_cursor();
    std::unique_ptr<graphics::GLContext> create_gl_context();

private:
    std::shared_ptr<NestedPlatform> const platform;
    std::shared_ptr<DisplayReport> const display_report;

    MirSurface* mir_surface;
    MirDisplayInfo egl_display_info;

    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
};

}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_DISPLAY_H_
