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
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/egl_resources.h"

#include "mir_toolkit/client_types.h"

#include <EGL/egl.h>

#include <mutex>
#include <unordered_map>

namespace mir
{
namespace input { class EventFilter; }
namespace geometry
{
struct Rectangle;
}
namespace graphics
{
class DisplayReport;
class DisplayBuffer;
class DisplayConfigurationPolicy;

namespace nested
{
namespace detail
{

class EGLSurfaceHandle
{
public:
    explicit EGLSurfaceHandle(EGLDisplay display, EGLNativeWindowType native_window, EGLConfig cfg);
    ~EGLSurfaceHandle() noexcept;

    operator EGLSurface() const { return egl_surface; }

private:
    EGLDisplay const egl_display;
    EGLSurface const egl_surface;
};

class EGLDisplayHandle
{
public:
    explicit EGLDisplayHandle(MirConnection* connection);
    ~EGLDisplayHandle() noexcept;

    void initialize(MirPixelFormat format);
    EGLConfig choose_windowed_es_config(MirPixelFormat format) const;
    EGLNativeWindowType native_window(EGLConfig egl_config, MirSurface* mir_surface) const;
    EGLContext egl_context() const;
    operator EGLDisplay() const { return egl_display; }

private:
    EGLDisplay egl_display;
    EGLContext egl_context_;

    EGLDisplayHandle(EGLDisplayHandle const&) = delete;
    EGLDisplayHandle operator=(EGLDisplayHandle const&) = delete;
};

class NestedOutput;

extern EGLint const nested_egl_context_attribs[];
}

class HostConnection;

class NestedDisplay : public Display
{
public:
    NestedDisplay(
        std::shared_ptr<HostConnection> const& connection,
        std::shared_ptr<input::EventFilter> const& event_handler,
        std::shared_ptr<DisplayReport> const& display_report,
        std::shared_ptr<DisplayConfigurationPolicy> const& conf_policy);

    ~NestedDisplay() noexcept;

    void for_each_display_buffer(std::function<void(DisplayBuffer&)>const& f) override;

    std::shared_ptr<DisplayConfiguration> configuration() override;
    void configure(DisplayConfiguration const&) override;

    void register_configuration_change_handler(
            EventHandlerRegister& handlers,
            DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(
            EventHandlerRegister& handlers,
            DisplayPauseHandler const& pause_handler,
            DisplayResumeHandler const& resume_handler) override;

    void pause() override;
    void resume() override;

    std::weak_ptr<Cursor> the_cursor() override;
    std::unique_ptr<graphics::GLContext> create_gl_context() override;

private:
    std::shared_ptr<HostConnection> const connection;
    std::shared_ptr<input::EventFilter> const event_handler;
    std::shared_ptr<DisplayReport> const display_report;
    detail::EGLDisplayHandle egl_display;

    std::mutex outputs_mutex;
    std::unordered_map<DisplayConfigurationOutputId, std::shared_ptr<detail::NestedOutput>> outputs;
    DisplayConfigurationChangeHandler my_conf_change_handler;
    void complete_display_initialization(MirPixelFormat format);
};

}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_DISPLAY_H_
