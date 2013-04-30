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
#include "mir/graphics/display_buffer.h"
#include "android_framebuffer_window.h"

#include <EGL/egl.h>
#include <memory>

namespace mir
{
namespace geometry
{
class Rectangle;
}
namespace graphics
{
class DisplayReport;
namespace android
{

class AndroidDisplay : public virtual Display, public virtual DisplayBuffer
{
public:
    explicit AndroidDisplay(std::shared_ptr<AndroidFramebufferWindowQuery> const&,
                            std::shared_ptr<DisplayReport> const&);
    ~AndroidDisplay();

    geometry::Rectangle view_area() const;
    void clear();
    void post_update();
    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f);

    std::shared_ptr<DisplayConfiguration> configuration();

    void register_pause_resume_handlers(
        MainLoop& main_loop,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler);

    void pause();
    void resume();

    void make_current();
    void release_current();

    std::weak_ptr<Cursor> the_cursor();

protected:
    EGLDisplay egl_display;
    EGLSurface egl_surface;
private:
    std::shared_ptr<AndroidFramebufferWindowQuery> native_window;
    EGLConfig egl_config;
    EGLContext egl_context;
    EGLContext egl_context_shared;
    EGLSurface egl_surface_dummy;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_DISPLAY_H_ */
