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

#include "display.h"
#include "display_buffer.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/virtual_output.h"
#include "mir/geometry/size.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgo = mg::offscreen;
namespace geom = mir::geometry;

namespace
{

mgo::detail::EGLDisplayHandle
create_and_initialize_display(EGLNativeDisplayType egl_native_display)
{
    mgo::detail::EGLDisplayHandle egl_display{egl_native_display};
    egl_display.initialize();
    return egl_display;
}

}

mgo::detail::EGLDisplayHandle::EGLDisplayHandle(EGLNativeDisplayType native_display)
    : egl_display{eglGetDisplay(native_display)}
{
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGL display"));
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
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialize EGL"));

    if ((major != 1) || (minor != 4))
        BOOST_THROW_EXCEPTION(std::runtime_error("EGL version 1.4 needed"));
}

mgo::detail::EGLDisplayHandle::~EGLDisplayHandle() noexcept
{
    if (egl_display != EGL_NO_DISPLAY)
        eglTerminate(egl_display);
}

mgo::detail::DisplaySyncGroup::DisplaySyncGroup(std::unique_ptr<mg::DisplayBuffer> output) :
    output(std::move(output))
{
}

void mgo::detail::DisplaySyncGroup::for_each_display_buffer(
    std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*output);
}

void mgo::detail::DisplaySyncGroup::post()
{
}

std::chrono::milliseconds
mgo::detail::DisplaySyncGroup::recommended_sleep() const
{
    return std::chrono::milliseconds::zero();
}

mgo::Display::Display(
    EGLNativeDisplayType egl_native_display,
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<DisplayReport> const&)
    : egl_display{create_and_initialize_display(egl_native_display)},
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

void mgo::Display::for_each_display_sync_group(
    std::function<void(mg::DisplaySyncGroup&)> const& f)
{
    std::lock_guard<std::mutex> lock{configuration_mutex};

    for (auto& dg_ptr : display_sync_groups)
        f(*dg_ptr);
}

std::unique_ptr<mg::DisplayConfiguration> mgo::Display::configuration() const
{
    std::lock_guard<std::mutex> lock{configuration_mutex};
    return std::make_unique<mgo::DisplayConfiguration>(
        current_display_configuration);
}

void mgo::Display::configure(mg::DisplayConfiguration const& conf)
{
    if (!conf.valid())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("Invalid or inconsistent display configuration"));
    }

    std::lock_guard<std::mutex> lock{configuration_mutex};

    display_sync_groups.clear();

    conf.for_each_output(
        [this] (DisplayConfigurationOutput const& output)
        {
            if (output.connected && output.preferred_mode_index < output.modes.size())
            {
                eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
                auto raw_db = new mgo::DisplayBuffer{
                    SurfacelessEGLContext{egl_display, egl_context_shared},
                    output.extents()};

                display_sync_groups.emplace_back(
                    new mgo::detail::DisplaySyncGroup(std::unique_ptr<mg::DisplayBuffer>(raw_db)));
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

std::shared_ptr<mg::Cursor> mgo::Display::create_hardware_cursor()
{
    return {};
}

std::unique_ptr<mir::renderer::gl::Context> mgo::Display::create_gl_context() const
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
    return std::make_unique<SurfacelessEGLContext>(egl_display, egl_context_shared);
}

mg::NativeDisplay* mgo::Display::native_display()
{
    return this;
}

mg::Frame mgo::Display::last_frame_on(unsigned) const
{
    return {};
}

std::unique_ptr<mg::VirtualOutput> mgo::Display::create_virtual_output(int /*width*/, int /*height*/)
{
    return nullptr;
}

bool mgo::Display::apply_if_configuration_preserves_display_buffers(mg::DisplayConfiguration const&)
{
    return false;
}
