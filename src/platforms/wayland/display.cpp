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

#include <epoxy/egl.h>

#include "display.h"
#include "cursor.h"

#include <mir/fatal.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/virtual_output.h>
#include <mir/log.h>
#include <mir/renderer/gl/context.h>

#include <boost/throw_exception.hpp>

#include <sys/eventfd.h>
#include <sys/poll.h>

namespace mgw = mir::graphics::wayland;
namespace mrg = mir::renderer::gl;
namespace geom= mir::geometry;

namespace
{
class NullInputSink : public mir::input::wayland::InputSinkX
{
    void key_press(std::chrono::nanoseconds, xkb_keysym_t, int32_t) override {}

    void key_release(std::chrono::nanoseconds, xkb_keysym_t, int32_t) override {}

    void pointer_press(
        std::chrono::nanoseconds, int, mir::geometry::Point const&,
        mir::geometry::Displacement) override
    {
    }

    void pointer_release(
        std::chrono::nanoseconds, int, mir::geometry::Point const&,
        mir::geometry::Displacement) override
    {
    }

    void pointer_motion(
        std::chrono::nanoseconds, mir::geometry::Point const&,
        mir::geometry::Displacement) override
    {
    }
};
}

mgw::Display* mgw::the_display = nullptr;

mgw::Display::Display(
    wl_display* const wl_display,
    std::shared_ptr<GLConfig> const& gl_config,
    std::shared_ptr<DisplayReport> const& report) :
    DisplayClient{wl_display, gl_config},
    report{report},
    shutdown_signal{::eventfd(0, EFD_CLOEXEC)},
    keyboard_sink{std::make_shared<NullInputSink>()},
    pointer_sink{std::make_shared<NullInputSink>()}
{
    runner = std::thread{[this] { run(); }};
    the_display = this;
}

auto mgw::Display::configuration() const -> std::unique_ptr<DisplayConfiguration>
{
    return DisplayClient::display_configuration();
}

void mgw::Display::configure(DisplayConfiguration const& /*conf*/)
{
    puts(__PRETTY_FUNCTION__);
}

void mgw::Display::register_configuration_change_handler(
    EventHandlerRegister& /*handlers*/,
    DisplayConfigurationChangeHandler const& /*conf_change_handler*/)
{
}

void mgw::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mgw::Display::pause()
{
   BOOST_THROW_EXCEPTION(std::runtime_error("'Display::pause()' not yet supported on wayland platform"));
}

void mgw::Display::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("'Display::resume()' not yet supported on wayland platform"));
}

auto mgw::Display::create_hardware_cursor() -> std::shared_ptr<Cursor>
{
    return std::make_shared<platform::wayland::Cursor>(display, std::shared_ptr<CursorImage>{});
}

auto mgw::Display::create_virtual_output(int /*width*/, int /*height*/) -> std::unique_ptr<VirtualOutput>
{
    return {};
}

auto mgw::Display::native_display() -> NativeDisplay*
{
    return this;
}

bool mgw::Display::apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& /*conf*/)
{
    return false;
}

auto mgw::Display::last_frame_on(unsigned) const -> Frame
{
    fatal_error(__PRETTY_FUNCTION__);
    return {};
}

auto mgw::Display::create_gl_context() const -> std::unique_ptr<mrg::Context>
{
    class GlContext : public mir::renderer::gl::Context
    {
    public:
        GlContext(EGLDisplay egldisplay, EGLConfig eglconfig, EGLContext eglctx) :
            egldisplay{egldisplay},
            eglctx{eglCreateContext(egldisplay, eglconfig, eglctx, nullptr)} {}

        void make_current()     const override
        {
            if (eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglctx) != EGL_TRUE)
            {
                log_warning(
                    "%s FAILED: %s",
                    __PRETTY_FUNCTION__,
                    egl_category().message(eglGetError()).c_str());
            }
        }

        void release_current()  const override
            { eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); }

    private:
        EGLDisplay const egldisplay;
        EGLContext const eglctx;
    };

    return std::make_unique<GlContext>(egldisplay, eglconfig, eglctx);
}

void mgw::Display::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    DisplayClient::for_each_display_sync_group(f);
}

void mgw::Display::set_keyboard_sink(std::shared_ptr<input::wayland::InputSinkX> const& keyboard_sink)
{
    std::lock_guard<decltype(sink_mutex)> lock{sink_mutex};
    if (keyboard_sink)
    {
        this->keyboard_sink = keyboard_sink;
    }
    else
    {
        this->keyboard_sink = std::make_shared<NullInputSink>();
    }
}

void mir::graphics::wayland::Display::keyboard_keymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
{
    DisplayClient::keyboard_keymap(keyboard, format, fd, size);
}

void mir::graphics::wayland::Display::keyboard_enter(
    wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys)
{
    DisplayClient::keyboard_enter(keyboard, serial, surface, keys);
}

void mir::graphics::wayland::Display::keyboard_leave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface)
{
    DisplayClient::keyboard_leave(keyboard, serial, surface);
}

void mir::graphics::wayland::Display::keyboard_key(wl_keyboard*, uint32_t, uint32_t time, uint32_t key, uint32_t state)
{
    auto const keysym = xkb_state_key_get_one_sym(keyboard_state(), key + 8);

    std::lock_guard<decltype(sink_mutex)> lock{sink_mutex};
    switch (state)
    {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
        keyboard_sink->key_press(std::chrono::milliseconds{time}, keysym, key);
        break;

    case WL_KEYBOARD_KEY_STATE_RELEASED:
        keyboard_sink->key_release(std::chrono::milliseconds{time}, keysym, key);
        break;
    }
}

void mir::graphics::wayland::Display::keyboard_modifiers(
    wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
    DisplayClient::keyboard_modifiers(keyboard, serial, mods_depressed, mods_latched, mods_locked, group);
}

void mir::graphics::wayland::Display::keyboard_repeat_info(wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
{
    DisplayClient::keyboard_repeat_info(wl_keyboard, rate, delay);
}

void mir::graphics::wayland::Display::run() const
{
    enum FdIndices {
        display_fd = 0,
        shutdown,
        indices
    };

    pollfd fds[indices] =
        {
            fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0},
            {shutdown_signal, POLLIN, 0},
        };

    while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
    {
        while (wl_display_prepare_read(display) != 0)
        {
            if (wl_display_dispatch_pending(display) == -1)
            {
                BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to dispatch Wayland events"}));
            }
        }

        if (poll(fds, indices, -1) == -1)
        {
            wl_display_cancel_read(display);
            BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to wait for event"}));
        }

        if (fds[display_fd].revents & (POLLIN | POLLERR))
        {
            if (wl_display_read_events(display))
            {
                BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to read Wayland events"}));
            }
        }
        else
        {
            wl_display_cancel_read(display);
        }
    }
}

void mir::graphics::wayland::Display::stop()
{
    if (eventfd_write(shutdown_signal, 1) == -1)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to shutdown"}));
    }
}

mir::graphics::wayland::Display::~Display()
{
    the_display = nullptr;
    stop();
    if (runner.joinable()) runner.join();
}

void mgw::Display::set_pointer_sink(std::shared_ptr<input::wayland::InputSinkX> const& pointer_sink)
{
    std::lock_guard<decltype(sink_mutex)> lock{sink_mutex};
    if (pointer_sink)
    {
        this->pointer_sink = pointer_sink;
    }
    else
    {
        this->pointer_sink = std::make_shared<NullInputSink>();
    }
}

void mir::graphics::wayland::Display::pointer_enter(
    wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
    DisplayClient::pointer_enter(pointer, serial, surface, x, y);
}

void mir::graphics::wayland::Display::pointer_leave(wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
    DisplayClient::pointer_leave(pointer, serial, surface);
}

void mir::graphics::wayland::Display::pointer_motion(wl_pointer*, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    std::lock_guard<decltype(sink_mutex)> lock{sink_mutex};
    geom::Point const new_pointer{wl_fixed_to_int(x), wl_fixed_to_int(y)};
    auto const movement = new_pointer - pointer;
    pointer = new_pointer;
    //            pointer_motion(std::chrono::nanoseconds event_time, geometry::Point const& pos, geometry::Displacement scroll) = 0;
    pointer_sink->pointer_motion(std::chrono::milliseconds{time}, pointer, movement);
}

void mir::graphics::wayland::Display::pointer_button(wl_pointer*, uint32_t, uint32_t time, uint32_t button, uint32_t state)
{
    std::lock_guard<decltype(sink_mutex)> lock{sink_mutex};

    switch (state)
    {
    case WL_POINTER_BUTTON_STATE_PRESSED:
        pointer_sink->pointer_press(std::chrono::milliseconds{time}, button, pointer, {});
        break;

    case WL_POINTER_BUTTON_STATE_RELEASED:
        pointer_sink->pointer_release(std::chrono::milliseconds{time}, button, pointer, {});
        break;
    }
}

void mir::graphics::wayland::Display::pointer_axis(wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    DisplayClient::pointer_axis(pointer, time, axis, value);
}

void mir::graphics::wayland::Display::pointer_frame(wl_pointer* pointer)
{
    DisplayClient::pointer_frame(pointer);
}

void mir::graphics::wayland::Display::pointer_axis_source(wl_pointer* pointer, uint32_t axis_source)
{
    DisplayClient::pointer_axis_source(pointer, axis_source);
}

void mir::graphics::wayland::Display::pointer_axis_stop(wl_pointer* pointer, uint32_t time, uint32_t axis)
{
    DisplayClient::pointer_axis_stop(pointer, time, axis);
}

void mir::graphics::wayland::Display::pointer_axis_discrete(wl_pointer* pointer, uint32_t axis, int32_t discrete)
{
    DisplayClient::pointer_axis_discrete(pointer, axis, discrete);
}
