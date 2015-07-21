/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_H_
#define MIR_GRAPHICS_X_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir_toolkit/common.h"
#include "display_group.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace X
{

class X11EGLDisplay
{
public:
    X11EGLDisplay(::Display *x_dpy);
    ~X11EGLDisplay();

    operator EGLDisplay() const;

private:
    EGLDisplay const egl_dpy;
};

class X11Window
{
public:
    X11Window(::Display* const x_dpy, EGLDisplay egl_dpy, int width, int height);
    ~X11Window();

    operator Window() const;
    EGLConfig egl_config() const;

private:
    ::Display* const x_dpy;
    Window win;
    EGLConfig config;
};

class X11EGLContext
{
public:
    X11EGLContext(EGLDisplay egl_dpy, EGLConfig config);
    ~X11EGLContext();

    operator EGLContext() const;

private:
    EGLContext egl_ctx;
    EGLDisplay const egl_dpy;
};

class X11EGLSurface
{
public:
    X11EGLSurface(EGLDisplay egl_dpy, EGLConfig config, Window win);
    ~X11EGLSurface();

    operator EGLSurface() const;

private:
    EGLDisplay const egl_dpy;
    EGLSurface const egl_surf;
};

class Display : public graphics::Display
{
public:
    explicit Display(::Display *dpy);
    ~Display() noexcept;

    void for_each_display_sync_group(std::function<void(graphics::DisplaySyncGroup&)> const& f) override;

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
    ::Display *x_dpy;
    X11EGLDisplay const egl_display;
    int const display_width;
    int const display_height;
    X11Window const win;
    X11EGLContext egl_context;
    X11EGLSurface egl_surface;
    MirPixelFormat pf;
    std::unique_ptr<DisplayGroup> display_group;
};

}
}
}
#endif /* MIR_GRAPHICS_X_DISPLAY_H_ */
