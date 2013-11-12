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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display.h"
#include "display_buffer.h"
#include "mir/graphics/offscreen_platform.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/gl_context.h"
#include "mir/geometry/size.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgo = mg::offscreen;
namespace geom = mir::geometry;

namespace
{

class OffscreenGLContext : public mg::GLContext
{
public:
    OffscreenGLContext(mgo::SurfacelessEGLContext surfaceless_egl_context)
        : surfaceless_egl_context{std::move(surfaceless_egl_context)}
    {
    }

    ~OffscreenGLContext() noexcept
    {
        release_current();
    }

    void make_current()
    {
        surfaceless_egl_context.make_current();
    }

    void release_current()
    {
        surfaceless_egl_context.release_current();
    }

private:
    mgo::SurfacelessEGLContext const surfaceless_egl_context;
};

mgo::detail::EGLDisplayHandle
create_and_initialize_display(mg::OffscreenPlatform& offscreen_platform)
{
    mgo::detail::EGLDisplayHandle egl_display{
        offscreen_platform.egl_native_display()};

    egl_display.initialize();

    return egl_display;
}

}

mgo::detail::EGLDisplayHandle::EGLDisplayHandle(EGLNativeDisplayType native_display)
    : egl_display{eglGetDisplay(native_display)}
{
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to get EGL display"));
}

mgo::detail::EGLDisplayHandle::EGLDisplayHandle(EGLDisplayHandle&& other)
    : egl_display{other.egl_display}
{
    other.egl_display = EGL_NO_DISPLAY;
}

void mgo::detail::EGLDisplayHandle::initialize()
{
    int major, minor;

    if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to initialize EGL"));

    if ((major != 1) || (minor != 4))
        BOOST_THROW_EXCEPTION(std::runtime_error("EGL version 1.4 needed"));
}

mgo::detail::EGLDisplayHandle::~EGLDisplayHandle() noexcept
{
    if (egl_display != EGL_NO_DISPLAY)
        eglTerminate(egl_display);
}

mgo::Display::Display(
    std::shared_ptr<OffscreenPlatform> const& offscreen_platform,
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<DisplayReport> const&)
    : offscreen_platform{offscreen_platform},
      egl_display{create_and_initialize_display(*offscreen_platform)},
      egl_context_shared{egl_display, EGL_NO_CONTEXT},
      current_display_configuration{geom::Size{1024,768}}
{
    /*
     * Make the shared context current. This needs to be done before we configure()
     * since mgo::DisplayBuffer creation needs a current GL context.
     */
    egl_context_shared.make_current();

    initial_conf_policy->apply_to(current_display_configuration);

    configure(current_display_configuration);
}

mgo::Display::~Display() noexcept
{
}

void mgo::Display::for_each_display_buffer(
    std::function<void(mg::DisplayBuffer&)> const& f)
{
    std::lock_guard<std::mutex> lock{configuration_mutex};

    for (auto& db_ptr : display_buffers)
        f(*db_ptr);
}

std::shared_ptr<mg::DisplayConfiguration> mgo::Display::configuration()
{
    std::lock_guard<std::mutex> lock{configuration_mutex};
    return std::make_shared<mgo::DisplayConfiguration>(current_display_configuration);
}

void mgo::Display::configure(mg::DisplayConfiguration const& conf)
{
    std::lock_guard<std::mutex> lock{configuration_mutex};

    display_buffers.clear();

    conf.for_each_output(
        [this] (DisplayConfigurationOutput const& output)
        {
            if (output.connected && output.preferred_mode_index < output.modes.size())
            {
                geom::Rectangle const area{
                    output.top_left, output.modes[output.current_mode_index].size};
                auto raw_db = new mgo::DisplayBuffer{
                    SurfacelessEGLContext{egl_display, egl_context_shared},
                    area};

                display_buffers.push_back(std::unique_ptr<mg::DisplayBuffer>(raw_db));
            }
        });
}

void mgo::Display::register_configuration_change_handler(
    EventHandlerRegister&,
    DisplayConfigurationChangeHandler const&)
{
}

void mgo::Display::register_pause_resume_handlers(
    EventHandlerRegister&,
    DisplayPauseHandler const&,
    DisplayResumeHandler const&)
{
}

void mgo::Display::pause()
{
}

void mgo::Display::resume()
{
}

std::weak_ptr<mg::Cursor> mgo::Display::the_cursor()
{
    return {};
}

std::unique_ptr<mg::GLContext> mgo::Display::create_gl_context()
{
    return std::unique_ptr<GLContext>{
        new OffscreenGLContext{SurfacelessEGLContext{egl_display, egl_context_shared}}};
}
