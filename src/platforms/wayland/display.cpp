/*
 * Copyright © Canonical Ltd.
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
#include "display_input.h"
#include "cursor.h"

#include <mir/fatal.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/egl_error.h>
#include <mir/log.h>
#include <mir/renderer/gl/context.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/throw_exception.hpp>

#include <sys/eventfd.h>
#include <poll.h>
#include <algorithm>
#include <vector>

namespace mgw = mir::graphics::wayland;
namespace mrg = mir::renderer::gl;
namespace geom= mir::geometry;

namespace
{
class NullKeyboardInput : public mir::input::wayland::KeyboardInput
{
    void key_press(std::chrono::nanoseconds, xkb_keysym_t, int32_t) override {}

    void key_release(std::chrono::nanoseconds, xkb_keysym_t, int32_t) override {}
};

class NullPointerInput : public mir::input::wayland::PointerInput
{
    void pointer_press(
        std::chrono::nanoseconds, int,
        geom::PointF const&,
        geom::DisplacementF const&) override
    {
    }

    void pointer_release(
        std::chrono::nanoseconds, int,
        geom::PointF const&,
        geom::DisplacementF const&) override
    {
    }

    void pointer_motion(
        std::chrono::nanoseconds,
        geom::PointF const&,
        geom::DisplacementF const&) override
    {
    }

    void pointer_axis_motion(
        MirPointerAxisSource,
        std::chrono::nanoseconds,
        mir::geometry::PointF const&,
        mir::geometry::DisplacementF const&) override
    {
    }
};

class NullTouchInput : public mir::input::wayland::TouchInput
{
    void touch_event(std::chrono::nanoseconds, std::vector<mir::events::TouchContact> const&) override
    {
    }
};

std::mutex the_display_mtx;
mgw::Display* the_display = nullptr;
}


mgw::Display::Display(
    wl_display* const wl_display,
    std::shared_ptr<WlDisplayTarget> target,
    std::shared_ptr<GLConfig> const&,
    std::shared_ptr<DisplayReport> const& report) :
    DisplayClient{wl_display, std::move(target)},
    report{report},
    shutdown_signal{::eventfd(0, EFD_CLOEXEC)},
    flush_signal{::eventfd(0, EFD_SEMAPHORE)},
    keyboard_sink{std::make_shared<NullKeyboardInput>()},
    pointer_sink{std::make_shared<NullPointerInput>()},
    touch_sink{std::make_shared<NullTouchInput>()}
{
    runner = std::thread{[this] { run(); }};

    {
        std::lock_guard lock{the_display_mtx};
        the_display = this;
    }
}

auto mgw::Display::configuration() const -> std::unique_ptr<DisplayConfiguration>
{
    return DisplayClient::display_configuration();
}

void mgw::Display::configure(DisplayConfiguration const& /*conf*/)
{
    delete_outputs_to_be_deleted();
}

void mgw::Display::register_configuration_change_handler(
    EventHandlerRegister& /*handlers*/,
    DisplayConfigurationChangeHandler const& conf_change_handler)
{
    std::lock_guard lock{config_change_handlers_mutex};
    config_change_handlers.push_back(conf_change_handler);
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
    cursor = std::make_shared<platform::wayland::Cursor>(compositor, shm, [this]{ flush_wl(); });
    return cursor;
}

bool mgw::Display::apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& /*conf*/)
{
    return false;
}

void mgw::Display::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    DisplayClient::for_each_display_sync_group(f);
}

void mgw::Display::set_keyboard_sink(std::shared_ptr<input::wayland::KeyboardInput> const& keyboard_sink)
{

    std::lock_guard display_lock{the_display_mtx};
    if (!the_display) fatal_error("mir::graphics::wayland::Display not initialized");

    std::lock_guard lock{the_display->sink_mutex};
    if (keyboard_sink)
    {
        the_display->keyboard_sink = keyboard_sink;
    }
    else
    {
        the_display->keyboard_sink = std::make_shared<NullKeyboardInput>();
    }
}

void mir::graphics::wayland::Display::keyboard_key(wl_keyboard*, uint32_t, uint32_t time, uint32_t key, uint32_t state)
{
    auto const keysym = xkb_state_key_get_one_sym(keyboard_state(), key + 8);

    std::lock_guard lock{sink_mutex};
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

void mir::graphics::wayland::Display::run()
try
{
    enum FdIndices {
        display_fd = 0,
        flush,
        shutdown,
        indices
    };

    pollfd fds[indices] =
        {
            fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0},
            {flush_signal, POLLIN, 0},
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

        if (wl_display_flush(display) < 0)
        {
            // wl_display_flush may fail with errno == EAGAIN if the write buffer is full
            if (errno == EAGAIN)
            {
                // In this case, we should also wake if the display_fd is writable, so we can
                // finish flushing.
                fds[display_fd].events |= POLLOUT;
            }
            else
            {
                BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to flush Wayland events"}));
            }
        }
        else
        {
            // We've fully flushed our pending events to the server; we don't need to wake-on-writable
            fds[display_fd].events &= ~POLLOUT;
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

        if (fds[flush].revents & (POLLIN | POLLERR))
        {
            eventfd_t foo;
            eventfd_read(flush_signal, &foo);
            std::unique_lock lock{workqueue_mutex};
            while (!workqueue.empty())
            {
                auto const work = std::move(workqueue.front());
                workqueue.pop_front();
                lock.unlock();
                work();
                lock.lock();
            }
            lock.unlock();
        }
    }
}
catch (std::exception const& e)
{
    fatal_error("Critical error in Wayland platform: %s\n", boost::diagnostic_information(e).c_str());
}

void mir::graphics::wayland::Display::flush_wl() const
{
    eventfd_write(flush_signal, 1);
}

void mir::graphics::wayland::Display::stop()
{
    flush_wl();
    if (eventfd_write(shutdown_signal, 1) == -1)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to shutdown"}));
    }
}

void mir::graphics::wayland::Display::spawn(std::function<void()>&& work)
{
    std::unique_lock lock{workqueue_mutex};
    workqueue.emplace_back(std::move(work));
    lock.unlock();
    flush_wl();
}

mir::graphics::wayland::Display::~Display()
{
    {
        std::lock_guard lock{the_display_mtx};
        the_display = nullptr;
    }
    stop();
    if (runner.joinable()) runner.join();
}

void mgw::Display::set_pointer_sink(std::shared_ptr<input::wayland::PointerInput> const& pointer_sink)
{
    std::lock_guard display_lock{the_display_mtx};
    if (!the_display) fatal_error("mir::graphics::wayland::Display not initialized");

    std::lock_guard lock{the_display->sink_mutex};
    if (pointer_sink)
    {
        the_display->pointer_sink = pointer_sink;
    }
    else
    {
        the_display->pointer_sink = std::make_shared<NullPointerInput>();
    }
}

void mir::graphics::wayland::Display::pointer_motion(wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    {
        std::lock_guard lock{sink_mutex};
        pointer_pos = geom::PointF{wl_fixed_to_double(x), wl_fixed_to_double(y)} + geom::DisplacementF{pointer_displacement};
        pointer_time = std::chrono::milliseconds{time};
    }

    DisplayClient::pointer_motion(pointer, time, x, y);
}

void mir::graphics::wayland::Display::pointer_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    {
        std::lock_guard lock{sink_mutex};
        pointer_time = std::chrono::milliseconds{time};

        switch (state)
        {
        case WL_POINTER_BUTTON_STATE_PRESSED:
            pointer_sink->pointer_press(pointer_time, button, pointer_pos, pointer_scroll);
            break;

        case WL_POINTER_BUTTON_STATE_RELEASED:
            pointer_sink->pointer_release(pointer_time, button, pointer_pos, pointer_scroll);
            break;
        }
        pointer_scroll = {};
    }
    DisplayClient::pointer_button(pointer, serial, time, button, state);
}

void mir::graphics::wayland::Display::pointer_axis(wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    {
        std::lock_guard lock{sink_mutex};
        switch (axis)
        {
        case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
            pointer_scroll.dx = geom::DeltaXF{wl_fixed_to_double(value)};
            break;

        case WL_POINTER_AXIS_VERTICAL_SCROLL:
            pointer_scroll.dy = geom::DeltaYF{wl_fixed_to_double(value)};
            break;
        }
    }

    DisplayClient::pointer_axis(pointer, time, axis, value);
}

void mir::graphics::wayland::Display::pointer_frame(wl_pointer* pointer)
{
    {
        std::lock_guard lock{sink_mutex};

        if (pointer_axis_source_ == mir_pointer_axis_source_none)
        {
            pointer_sink->pointer_motion(pointer_time, pointer_pos, pointer_scroll);
        }
        else
        {
            pointer_sink->pointer_axis_motion(pointer_axis_source_, pointer_time, pointer_pos, pointer_scroll);
        }

        pointer_axis_source_ = mir_pointer_axis_source_none;
        pointer_scroll = {};
    }

    DisplayClient::pointer_frame(pointer);
}

void mir::graphics::wayland::Display::touch_down(
    wl_touch* touch, uint32_t serial, uint32_t time, wl_surface* surface, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    DisplayClient::touch_down(touch, serial, time, surface, id, x, y);

    {
        std::lock_guard lock{sink_mutex};
        auto const contact = get_touch_contact(id);

        touch_time = std::chrono::milliseconds{time};
        contact->action = mir_touch_action_down;
        contact->position = geom::PointF{
            wl_fixed_to_double(x) + touch_displacement.dx.as_int(),
            wl_fixed_to_double(y) + touch_displacement.dy.as_int()};
    }
}

void mir::graphics::wayland::Display::touch_up(wl_touch* touch, uint32_t serial, uint32_t time, int32_t id)
{
    {
        std::lock_guard lock{sink_mutex};
        auto const contact = get_touch_contact(id);

        touch_time = std::chrono::milliseconds{time};
        contact->action = mir_touch_action_up;

        touch_sink->touch_event(touch_time, touch_contacts);
        touch_contacts.erase(contact);
    }

    DisplayClient::touch_up(touch, serial, time, id);
}

void mir::graphics::wayland::Display::touch_motion(wl_touch* touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    {
        std::lock_guard lock{sink_mutex};
        auto const contact = get_touch_contact(id);

        touch_time = std::chrono::milliseconds{time};
        contact->action = mir_touch_action_change;
        contact->position = geom::PointF{
            wl_fixed_to_double(x) + touch_displacement.dx.as_int(),
            wl_fixed_to_double(y) + touch_displacement.dy.as_int()};
    }

    DisplayClient::touch_motion(touch, time, id, x, y);
}

void mir::graphics::wayland::Display::touch_frame(wl_touch* touch)
{
    {
        std::lock_guard lock{sink_mutex};

        if (touch_contacts.size())
        {
            touch_sink->touch_event(touch_time, touch_contacts);
        }
    }

    DisplayClient::touch_frame(touch);
}

void mir::graphics::wayland::Display::touch_cancel(wl_touch* touch)
{
    {
        std::lock_guard lock{sink_mutex};

        for (auto& contact : touch_contacts)
        {
            contact.action = mir_touch_action_up;
        }

        if (touch_contacts.size())
        {
            touch_sink->touch_event(touch_time, touch_contacts);
            touch_contacts.resize(0);
        }
    }

    DisplayClient::touch_cancel(touch);
}

void mir::graphics::wayland::Display::touch_shape(wl_touch* touch, int32_t id, wl_fixed_t major, wl_fixed_t minor)
{
    {
        std::lock_guard lock{sink_mutex};
        auto const contact = get_touch_contact(id);

        contact->action = mir_touch_action_change;
        contact->touch_major = wl_fixed_to_double(major);
        contact->touch_minor = wl_fixed_to_double(minor);
    }

    DisplayClient::touch_shape(touch, id, major, minor);
}

void mir::graphics::wayland::Display::touch_orientation(wl_touch* touch, int32_t id, wl_fixed_t orientation)
{
    {
        std::lock_guard lock{sink_mutex};
        auto const contact = get_touch_contact(id);

        contact->action = mir_touch_action_change;
        contact->orientation = wl_fixed_to_double(orientation);
    }

    DisplayClient::touch_orientation(touch, id, orientation);
}

void mir::graphics::wayland::Display::set_touch_sink(std::shared_ptr<input::wayland::TouchInput> const& touch_sink)
{
    std::lock_guard display_lock{the_display_mtx};
    if (!the_display) fatal_error("mir::graphics::wayland::Display not initialized");

    std::lock_guard lock{the_display->sink_mutex};
    if (touch_sink)
    {
        the_display->touch_sink = touch_sink;
    }
    else
    {
        the_display->touch_sink = std::make_shared<NullTouchInput>();
    }
}

auto mir::graphics::wayland::Display::get_touch_contact(int32_t id) -> decltype(touch_contacts)::iterator
{
    auto contact = std::find_if(std::begin(this->touch_contacts), std::end(this->touch_contacts), [&](auto& c){ return c.touch_id == id; });

    if (contact == std::end(touch_contacts))
    {
        touch_contacts.resize(touch_contacts.size() + 1);
        contact = std::end(touch_contacts) - 1;
        contact->touch_id = id;
        contact->tooltype = mir_touch_tooltype_unknown;
    }
    return contact;
}

void mir::graphics::wayland::Display::pointer_enter(
    wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
    if (cursor) cursor->enter(pointer);
    DisplayClient::pointer_enter(pointer, serial, surface, x, y);
}

void mir::graphics::wayland::Display::pointer_leave(wl_pointer* pointer, uint32_t serial, wl_surface* surface)
{
    if (cursor) cursor->leave(pointer);
    DisplayClient::pointer_leave(pointer, serial, surface);
}

bool mir::graphics::wayland::Display::active()
{
    std::lock_guard display_lock{the_display_mtx};
    return the_display != nullptr;
}

void mir::graphics::wayland::Display::pointer_axis_source(wl_pointer* /*pointer*/, uint32_t axis_source)
{
    switch (axis_source)
    {
    case WL_POINTER_AXIS_SOURCE_WHEEL:
        pointer_axis_source_ = mir_pointer_axis_source_wheel;
        break;

    case WL_POINTER_AXIS_SOURCE_FINGER:
        pointer_axis_source_ = mir_pointer_axis_source_finger;
        break;

    case WL_POINTER_AXIS_SOURCE_CONTINUOUS:
        pointer_axis_source_ = mir_pointer_axis_source_continuous;
        break;

    case WL_POINTER_AXIS_SOURCE_WHEEL_TILT:
        pointer_axis_source_ = mir_pointer_axis_source_wheel_tilt;
        break;

    default:
        break;
    }
}
