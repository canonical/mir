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
#include "mir/graphics/egl_resources.h"

#include "mir_toolkit/mir_client_library.h"

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
namespace detail
{
class MirSurfaceHandle
{
public:
    explicit MirSurfaceHandle(MirConnection* connection, MirDisplayOutput* const egl_display_info);
    ~MirSurfaceHandle() noexcept;

    operator MirSurface*() const { return mir_surface; }

private:
    MirSurface* mir_surface;

    MirSurfaceHandle(MirSurfaceHandle const&) = delete;
    MirSurfaceHandle operator=(MirSurfaceHandle const&) = delete;
};

class EGLDisplayHandle
{
public:
    explicit EGLDisplayHandle(MirConnection* connection);
    ~EGLDisplayHandle() noexcept;

    void initialize() const;
    EGLConfig choose_config(const EGLint attrib_list[]) const;
    EGLSurface egl_surface(EGLConfig egl_config, MirSurface* mir_surface) const;

    operator EGLDisplay() const { return egl_display; }

private:
    EGLDisplay egl_display;

    EGLDisplayHandle(EGLDisplayHandle const&) = delete;
    EGLDisplayHandle operator=(EGLDisplayHandle const&) = delete;
};

class NestedOutput
{
public:
    NestedOutput(MirConnection* connection, MirDisplayOutput* const);
    ~NestedOutput() noexcept;

    operator MirSurface*() const { return mir_surface; }
    operator EGLContext()  const { return egl_surface; }

    NestedOutput(NestedOutput const&) = delete;
    NestedOutput operator=(NestedOutput const&) = delete;
private:
    detail::MirSurfaceHandle const mir_surface;
    detail::EGLDisplayHandle const egl_display;

    EGLConfig const egl_config;
    EGLSurfaceStore const egl_surface;
    EGLContextStore const egl_context;
};

}

class NestedDisplay : public Display
{
public:
    NestedDisplay(MirConnection* connection, std::shared_ptr<DisplayReport>const& display_report);
    virtual ~NestedDisplay() noexcept;

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

    void pause();
    void resume();

    std::weak_ptr<Cursor> the_cursor();
    std::unique_ptr<graphics::GLContext> create_gl_context();

private:
    std::shared_ptr<DisplayReport> const display_report;

    // TODO support multiple outputs
    std::shared_ptr<detail::NestedOutput> const one_output;
};

}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_DISPLAY_H_
